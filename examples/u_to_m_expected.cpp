struct Point {
    int m_x;
    int m_y;
    int z_value;

    void reset() {
        m_x = 0;
        m_y = 0;
        const char c = '_';
        const char *s = "x_ should stay in strings";
    }
};
