#include "demo.h"

Demo::Demo(int m_member)
    : member_(m_member)
{
}

int Demo::add(int n) {
    return member_ + n;
};
