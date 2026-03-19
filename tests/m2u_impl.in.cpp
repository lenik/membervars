#include "demo.h"

Demo::Demo(int m_member)
    : m_member(m_member)
{
}

int Demo::add(int n) {
    return m_member + n;
};
