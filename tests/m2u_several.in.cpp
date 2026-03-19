class Container {
    int m_a;
    int m_b;
    int m_c;

public:
    void init(int x, int y) {
        this->m_a = x;
        this->m_b = y;
    }
    int sum() const { return m_a + m_b + m_c; }
};
