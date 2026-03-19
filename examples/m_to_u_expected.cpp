class Widget {
public:
    int value_;
    int count42_;
    int other_name;

    void set() {
        value_ = count42_;
        const char *s = "m_value should stay in strings";
        // m_count42 should stay in comments
        /* m_value stays here too */
    }
};
