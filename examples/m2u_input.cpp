class Widget {
public:
    int m_value;
    int m_count42;
    int other_name;

    void set() {
        m_value = m_count42;
        const char *s = "m_value should stay in strings";
        // m_count42 should stay in comments
        /* m_value stays here too */
    }
};
