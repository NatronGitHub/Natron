#ifndef __INTEGER_BOX_H__
#define __INTEGER_BOX_H__

class IntegerBox{
private:
	int x_; /*!< Left edge */
	int y_; /*!< Bottom edge */
	int r_; /*!< Right edge */
	int t_; /*!< Top edge */
    
public:
    
    /* box iterator, used to iterate over the area of a box.
     /y/ and /x/ are defined at each position.  Ordering will
     be bottom-to-top, left-to-right */
    class  iterator {
    public:
        int y;
        int x;
        
        iterator(int y, int x, int l, int r) : y(y), x(x), l(l), r(r) { }
        
        void operator++() {
            x++;
            if (x == r) {
                x = l;
                y++;
            }
        }
        
        void operator++(int) {
            x++;
            if (x == r) {
                x = l;
                y++;
            }
        }
        
        bool operator!=(const iterator& other) const
        {
            return y != other.y || x != other.x;
        }
        
        
    private:
        int l;
        int r;
    };
    
    typedef iterator const_iterator;
    
    iterator begin() const
    {
        return iterator(y_, x_, x_, r_);
    }
    
    iterator end() const
    {
        return iterator(t_, x_, x_, r_);
    }
    
    IntegerBox() : x_(0), y_(0), r_(1), t_(1) {}
    
    IntegerBox(int x, int y, int r, int t) : x_(x), y_(y), r_(r), t_(t) {}
    
    IntegerBox(const IntegerBox &b) { *this = b; }
    
    int x() const { return x_; } /*!< Location of left edge */
    void x(int v) { x_ = v; } /*!< Set left edge */
    
    int y() const { return y_; } /*!< Location of bottom edge */
    void y(int v) { y_ = v; } /*!< Set bottom edge */
    
    int range() const { return r_; } /*!< Location of right edge */
    void range(int v) { r_ = v; } /*!< Set right edge */
    
    int top() const { return t_; } /*!< Location of top edge */
    void top(int v) { t_ = v; } /*!< Set top edge */
    
    int w() const { return r_ - x_; } /*!< width, r()-x() */
    void w(int v) { r_ = x_ + v; } /*!< Set width by moving right edge */
    
    int h() const { return t_ - y_; } /*!< height, t()-y() */
    void h(int v) { t_ = y_ + v; } /*!< Set height by moving top edge */
    
    float center_x() const { return (x_ + r_) / 2.0f; } /*!< center in x */
    
    float center_y() const { return (y_ + t_) / 2.0f; } /*!< center in y */
    
    /*! Set all four sides at once. */
    void set(int x, int y, int r, int t) { x_ = x;
        y_ = y;
        r_ = r;
        t_ = t; }
    
    /*! Set all four sides at once. */
    void set(const IntegerBox& b) { *this = b; }
    
    /*! True if box is 1x1 in both directions. */
    bool is_constant() const { return r_ <= x_ + 1 && t_ <= y_ + 1; }
    
    void clear() { x_ = y_ = 0;
        r_ = t_ = 1; }        /*!< Set to is_constant() */
    
    /*! Move all the sides and thus the entire box by the given deltas */
    void move(int dx, int dy) { x_ += dx;
        r_ += dx;
        y_ += dy;
        t_ += dy; }
    
    /*! Move x(), y(), r(), t() by the given deltas, except if a
     dimension is 1 it does not move those edges, because the color is
     constant in that direction. This is used to pad out an input image
     with the spill that a filter will create. Notice that \a dx and \a
     dy are negative to make the box bigger. */
    void pad(int dx, int dy, int dr, int dt)
    {
        if (r_ > x_ + 1) { x_ += dx;
            r_ += dr; }
        if (t_ > y_ + 1) { y_ += dy;
            t_ += dt; }
    }
    
    /*! Same as pad(-dx,-dy,dx,dy), add the same amount on all sides. */
    void pad(int dx, int dy) { pad(-dx, -dy, dx, dy); }
    
    /*! Same as pad(-d,-d,d,d), add the same amount on all sides. */
    void pad(int d) { pad(-d, -d, d, d); }
    
    /*! Return x restricted to pointing at a pixel in the box. */
    int clampx(int x) const { return x <= x_ ? x_ : x >= r_ ? r_ - 1 : x; }
    
    /*! Return y restricted to pointing at a pixel in the box. */
    int clampy(int y) const { return y <= y_ ? y_ : y >= t_ ? t_ - 1 : y; }
    
    void merge(const IntegerBox);
    
    void merge(int x, int y);
    
    void merge(int x, int y, int r, int t);
    
    void intersect(const IntegerBox);
    
    void intersect(int x, int y, int r, int t);
    
    bool isContained(const IntegerBox);
    
    bool isContained(int x,int y,int r,int t);
    
    int area() const {
        return w() * h();
    }
    IntegerBox& operator=(const IntegerBox& other){
        x_ = other.x();
        y_ = other.y();
        r_ = other.range();
        t_ = other.top();
        return *this;
    }
};
/// equality of boxes
inline bool operator==(const IntegerBox& b1, const IntegerBox& b2)
{
	return b1.x() == b2.x() &&
    b1.y() == b2.y() &&
    b1.range() == b2.range() &&
    b1.top() == b2.top();
}

/// inequality of boxes
inline bool operator!=(const IntegerBox& b1, const IntegerBox& b2)
{
	return b1.x() != b2.x() ||
    b1.y() != b2.y() ||
    b1.range() != b2.range() ||
    b1.top() != b2.top();
}

#endif