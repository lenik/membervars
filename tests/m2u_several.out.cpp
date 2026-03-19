class Container {
    int a_;
    int b_;
    int c_;

public:
    void init(int x, int y) {
        this->a_ = x;
        this->b_ = y;
    }
    int sum() const { return a_ + b_ + c_; }
};
