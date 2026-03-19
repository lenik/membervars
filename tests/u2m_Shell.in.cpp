#include "Shell.hpp"

#include "../core/App.hpp"
#include "../core/ModuleRegistry.hpp"
#include "../core/ServiceManager.hpp"
#include "../mod/explorer/ExplorerApp.hpp"

#include <bas/volume/VolumeManager.hpp>

#include <wx/image.h>
#include <wx/log.h>
#include <wx/splitter.h>
#include <wx/statline.h>

namespace os {

ShellApp* ShellApp::s_instance = nullptr;

ShellApp::ShellApp(std::string name)
    : name_(std::move(name))
    , volumeManager_(nullptr)
    , moduleRegistry_(nullptr)
    , mainWindow_(nullptr)
    , desktop_(nullptr)
    , taskbar_(nullptr)
    , startMenu_(nullptr) {
    s_instance = this;
}

ShellApp::~ShellApp() {
    ServiceManager::getInstance().stopAllServices();
    if (moduleRegistry_) {
        moduleRegistry_->uninstallAll();
        delete moduleRegistry_;
        moduleRegistry_ = nullptr;
    }
    s_instance = nullptr;
}

bool ShellApp::OnUserInit() {
    volumeManager_ = app.volumeManager.get();
    if (!volumeManager_) {
        wxLogError("VolumeManager not initialized");
        return false;
    }

    SetAppName(name_);
    SetAppDisplayName(name_ + " Desktop Environment");
    SetVendorName(name_ + " Project");

    moduleRegistry_ = new ModuleRegistry(volumeManager_);

    if (!initializeModules()) {
        wxLogError("Failed to initialize modules");
        return false;
    }

    createUI();
    setupEventHandlers();
    ServiceManager::getInstance().startAllServices(*moduleRegistry_);
    refreshDesktop();

    return true;
}

int ShellApp::OnExit() {
    wxLogInfo("Shell exiting");
    return wxApp::OnExit();
}

ShellApp* ShellApp::getInstance() { return s_instance; }

void ShellApp::launchModule(ModulePtr module) {
    if (!module) {
        wxLogError("Cannot launch null module");
        return;
    }

    if (!module->isEnabled()) {
        wxLogWarning("Module %s is not enabled", module->getFullUri());
        wxMessageBox("Module is not enabled", "Error", wxOK | wxICON_WARNING);
        return;
    }

    try {
        wxLogInfo("Launching module: %s", module->label);
        module->recordExecution();
        ProcessPtr p = module->run();
        if (p && taskbar_) {
            wxWindow* w = p->primaryWindow();
            if (w) {
                taskbar_->addProcess(p);
            }
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to launch module %s: %s", module->getFullUri(), e.what());
        wxMessageBox("Failed to launch " + module->label + ": " + e.what(), "Error",
                     wxOK | wxICON_ERROR);
    }
}

void ShellApp::refreshDesktop() {
    if (desktop_) {
        desktop_->clearIcons();
        desktop_->addVolumeIcons();
        desktop_->loadBackgroundSettings();
        if (moduleRegistry_) {
            for (auto& module : moduleRegistry_->getVisibleModules()) {
                desktop_->addIcon(module);
            }
        }
        desktop_->arrangeIcons();
        desktop_->loadLayout();
    }
}

bool ShellApp::initializeModules() {
    wxLogInfo("Initializing module system");

    // Install all registered modules
    if (moduleRegistry_) {
        moduleRegistry_->installAll();
    }

    return true;
}

void ShellApp::createUI() {
    wxLogInfo("Creating shell UI");

    // Create main frame
    mainWindow_ = new wxFrame(nullptr, wxID_ANY, "OmniShell", //
                              wxDefaultPosition, wxSize(1024, 768));
    mainWindow_->SetMinSize(wxSize(800, 600));
    mainWindow_->SetBackgroundColour(*wxWHITE);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Layer 1: icons layer (desktop)
    desktop_ = new DesktopWindow(mainWindow_);
    desktop_->setVolumeManager(volumeManager_);
    mainSizer->Add(desktop_, 1, wxEXPAND);

    // Layer 2: control layer (separator + taskbar)
    wxPanel* controlPanel = new wxPanel(mainWindow_);
    wxBoxSizer* controlSizer = new wxBoxSizer(wxVERTICAL);

    controlSizer->Add(new wxStaticLine(controlPanel, wxID_ANY), 0, wxEXPAND);

    taskbar_ = new Taskbar(controlPanel);
    controlSizer->Add(taskbar_, 0, wxEXPAND);

    controlPanel->SetSizer(controlSizer);
    mainSizer->Add(controlPanel, 0, wxEXPAND);

    // Start menu as overlay (not sizer-managed) so it can expand without squeezing GTK widgets.
    startMenu_ = new StartMenu(mainWindow_);
    startMenu_->Hide();

    mainWindow_->SetSizer(mainSizer);

    if (moduleRegistry_) {
        startMenu_->populateModules(moduleRegistry_->getVisibleModules());
    }

    // Click outside start menu -> hide
    auto hideStartIfOutside = [this](wxWindow* source, wxMouseEvent& e) {
        if (!startMenu_ || !startMenu_->IsShown()) {
            e.Skip();
            return;
        }
        wxPoint screenClick = source->ClientToScreen(e.GetPosition());
        if (!startMenu_->ContainsScreenPoint(screenClick)) {
            startMenu_->HideMenu();
        }
        e.Skip();
    };

    mainWindow_->Bind(wxEVT_LEFT_DOWN, [this, hideStartIfOutside](wxMouseEvent& e) {
        hideStartIfOutside(mainWindow_, e);
    });
    if (desktop_) {
        desktop_->Bind(wxEVT_LEFT_DOWN, [this, hideStartIfOutside](wxMouseEvent& e) {
            hideStartIfOutside(desktop_, e);
        });
        desktop_->Bind(wxEVT_RIGHT_DOWN, [this, hideStartIfOutside](wxMouseEvent& e) {
            hideStartIfOutside(desktop_, e);
        });
    }
    if (taskbar_) {
        taskbar_->Bind(wxEVT_LEFT_DOWN, [this, hideStartIfOutside](wxMouseEvent& e) {
            hideStartIfOutside(taskbar_, e);
        });
        taskbar_->Bind(wxEVT_RIGHT_DOWN, [this, hideStartIfOutside](wxMouseEvent& e) {
            hideStartIfOutside(taskbar_, e);
        });
    }

    // When start menu is visible, route character input to it so typing opens the search box.
    mainWindow_->Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
        if (startMenu_ && startMenu_->IsShown()) {
            if (startMenu_->HandleGlobalKey(e))
                return;
        }
        e.Skip();
    });

    mainWindow_->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
        if (startMenu_ && startMenu_->IsShown())
            positionStartMenu();
        e.Skip();
    });

    mainWindow_->CenterOnScreen();
    mainWindow_->Show(true);

    wxLogInfo("Shell UI created");
}

void ShellApp::setupEventHandlers() {
    // Setup start menu launch callback
    if (startMenu_) {
        startMenu_->setLaunchCallback([this](ModulePtr module) { launchModule(module); });
    }
}

void ShellApp::openExplorerAt(const std::string& dir) {
    ModuleRegistry* mr = moduleRegistry_;
    if (!mr || !volumeManager_)
        return;

    Volume* vol = volumeManager_->getDefaultVolume();
    if (!vol)
        return;

    // Ensure the directory exists; ignore errors, Explorer will report if it cannot open.
    const std::string path = "/" + dir;
    try {
        if (!vol->exists(path)) {
            vol->createDirectory(path);
        }
    } catch (...) {
    }

    // Open via Explorer application helper.
    ProcessPtr p = ExplorerApp::open(vol, path);
    if (p && taskbar_) {
        taskbar_->addProcess(p);
    }
}

void ShellApp::toggleStartMenu() {
    if (!startMenu_ || !mainWindow_)
        return;
    if (startMenu_->IsShown()) {
        startMenu_->HideMenu();
        return;
    }
    startMenu_->InvalidateBestSize();
    startMenu_->Layout();
    positionStartMenu();
    startMenu_->ShowMenu();
}

void ShellApp::positionStartMenu() {
    if (!startMenu_ || !mainWindow_ || !taskbar_)
        return;

    const wxSize frameClient = mainWindow_->GetClientSize();
    const int taskbarH = taskbar_->GetSize().GetHeight() > 0 ? taskbar_->GetSize().GetHeight()
                                                             : Taskbar::GetDefaultHeight();
    // Prefer the control's best (recommended) size rather than min size.
    startMenu_->Layout();
    wxSize bestSize = startMenu_->GetBestSize();
    int menuW = bestSize.GetWidth() > 0 ? bestSize.GetWidth() : startMenu_->GetSize().GetWidth();
    if (menuW <= 0)
        menuW = 280;

    // Height: fit content, but do not exceed the area above the taskbar.
    const int maxH = std::max(0, frameClient.GetHeight() - taskbarH);
    if (maxH <= 0)
        return;
    int desiredH = bestSize.GetHeight() > 0 ? bestSize.GetHeight() : startMenu_->GetSize().GetHeight();
    if (desiredH <= 0)
        desiredH = 420;
    int menuH = std::min(desiredH, maxH);

    // Width clamp: keep within client width for GTK stability.
    if (menuW > frameClient.GetWidth())
        menuW = frameClient.GetWidth();
    if (menuW < 60)
        menuW = 60;

    const int x = 0;
    int y = frameClient.GetHeight() - taskbarH - menuH;
    if (y < 0)
        y = 0;

    startMenu_->SetSize(wxSize(menuW, menuH));
    startMenu_->SetPosition(wxPoint(x, y));
    startMenu_->Raise();
}

} // namespace os
