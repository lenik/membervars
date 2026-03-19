struct Point {
    int x_;
    int y_;
    int z_value;

    void reset() {
        x_ = 0;
        y_ = 0;
        const char c = '_';
        const char *s = "x_ should stay in strings";
    }
};
