#ifndef WALL_HPP
#define WALL_HPP

class Wall {
public:
    enum class Orientation {
        HORIZONTAL, VERTICAL, CORNER
    };


    // Getters
    int getHitCount() const;
    bool getIsDestroyed() const;
    int getRow() const;
    int getCol() const;
    Orientation getOrientation() const;

    // Setters
    void setHitCount(int hitCount);
    void setIsDestroyed(bool isDestroyed);
    void setRow(int row);
    void setCol(int col);
    void setOrientation(Orientation orientation);

    // Physical override
    void update(float elapsed) noexcept;

    // Drawable override
    void draw(float elapsed) const noexcept;

private:

    int hitCount;
    bool isDestroyed;
    int row;
    int col;
    Orientation orientation;

};

#endif // WALL_HPP
