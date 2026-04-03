#include <algorithm>
#include <raylib.h>
#include <raymath.h>
#include <vector>

constexpr unsigned int wndWidth{800}, wndHeight{600};

class Ball {
public:
    static constexpr Color defColor{BLUE}; // "def" is for "default"
    static constexpr float defRadius{10.f};
    static constexpr float defVelocity{3.f};

    Vector2 position{0, 0};
    Vector2 velocity{-defVelocity, -defVelocity};

    Ball(float mX, float mY) {
        position = (Vector2){mX, mY};
    }

    float x() const noexcept { return position.x; }
    float y() const noexcept { return position.y; }
    float left() const noexcept { return x() - defRadius; }
    float right() const noexcept { return x() + defRadius; }
    float top() const noexcept { return y() - defRadius; }
    float bottom() const noexcept { return y() + defRadius; }

    void update() {
        position = Vector2Add(position, velocity); // "always move the object regardless and then solve the collision"
        solveBoundCollisions();
    }

    void draw() {
        DrawCircle(position.x, position.y, defRadius, defColor);
    }

private:
    void solveBoundCollisions() noexcept {
        if (left() < 0)
            velocity.x = defVelocity;
        else if (right() > wndWidth)
            velocity.x = -defVelocity;
        
        if (top() < 0)
            velocity.y = defVelocity;
        else if (bottom() > wndHeight)
            velocity.y = -defVelocity;
    }
};

class Paddle {
public:
    static constexpr Color defColor{MAGENTA};
    static constexpr float defWidth{60.f};
    static constexpr float defHeight{20.f};
    static constexpr float defVelocity{8.f};

    Vector2 position{0, 0};
    Vector2 velocity{0, 0};

    Paddle(float mX, float mY) {
        position = (Vector2){mX, mY};
    }

    float x() const noexcept { return position.x; }
    float y() const noexcept { return position.y; }
    float left() const noexcept { return x() - (defWidth / 2.f); }
    float right() const noexcept { return x() + (defWidth / 2.f); }
    float top() const noexcept { return y() - (defHeight / 2.f); }
    float bottom() const noexcept { return y() + (defHeight / 2.f); }

    void update() {
        processPlayerInput();
        position = Vector2Add(position, velocity);
    }

    void draw() {
        DrawRectanglePro(
            (Rectangle){position.x, position.y, defWidth, defHeight},
            (Vector2){defWidth / 2.f, defHeight / 2.f},
            0, defColor
        );
    }

private:
    void processPlayerInput() {
        if (IsKeyDown(KEY_LEFT) && left() > 0)
            velocity.x = -defVelocity;
        else if (IsKeyDown(KEY_RIGHT) && right() < wndWidth)
            velocity.x = defVelocity;
        else
            velocity.x = 0;
    }
};

class Brick {
public:
    static constexpr Color defColor{RED};
    static constexpr float defWidth{60.f};
    static constexpr float defHeight{20.f};

    bool destroyed{false};
    Vector2 position;

    Brick(float mX, float mY) {
        position = (Vector2){mX, mY};
    }

    float x() const noexcept { return position.x; }
    float y() const noexcept { return position.y; }
    float left() const noexcept { return x() - (defWidth / 2.f); }
    float right() const noexcept { return x() + (defWidth / 2.f); }
    float top() const noexcept { return y() - (defHeight / 2.f); }
    float bottom() const noexcept { return y() + (defHeight / 2.f); }

    void update() {}

    void draw() {
        DrawRectanglePro(
            (Rectangle){position.x, position.y, defWidth, defHeight},
            (Vector2){defWidth / 2.f, defHeight / 2.f},
            0, defColor
        );
    }
};

template <typename T1, typename T2>
bool isIntersecting(const T1& mA, const T2& mB) noexcept {
    // AABB vs AABB collision (Axis-Aligned Bounding Boxes)
    return
        mA.right() >= mB.left() && mA.left() <= mB.right() &&
        mA.bottom() >= mB.top() && mA.top() <= mB.bottom();
}

void solvePaddleBallCollision(const Paddle& mPaddle, Ball& mBall) noexcept {
    if (!isIntersecting(mPaddle, mBall)) return;

    mBall.velocity.y = -Ball::defVelocity;
    mBall.velocity.x =
        mBall.x() < mPaddle.x() ? -Ball::defVelocity : Ball::defVelocity;
}

void solveBrickBallCollision(Brick& mBrick, Ball& mBall) noexcept {
    if (!isIntersecting(mBrick, mBall)) return;

    mBrick.destroyed = true;

    // Calculate how much the ball intersects the brick in every direction.
    float overlapLeft{mBall.right() - mBrick.left()};
    float overlapRight{mBrick.right() - mBall.left()};
    float overlapTop{mBall.bottom() - mBrick.top()};
    float overlapBottom{mBrick.bottom() - mBall.top()};

    // If the magnitude of the left overlap is smaller than the right one,
    // we can safely assume the ball hit the brick from the left.
    // If the magnitude of the X overlap is less than the Y, we can safely assume
    // the ball hit the brick horizontally, and vertically otherwise.

    bool ballFromLeft(std::abs(overlapLeft) < std::abs(overlapRight));
    bool ballFromTop(std::abs(overlapTop) < std::abs(overlapBottom));

    float minOverlapX{ballFromLeft ? overlapLeft : overlapRight};
    float minOverlapY{ballFromTop ? overlapTop : overlapBottom};

    if (std::abs(minOverlapX) < std::abs(minOverlapY))
        mBall.velocity.x = ballFromLeft ? -Ball::defVelocity : Ball::defVelocity;
    else
        mBall.velocity.y = ballFromTop ? -Ball::defVelocity : Ball::defVelocity;
}

int main() {
    Ball ball{wndWidth / 2.f, wndHeight / 2.f};
    Paddle paddle{wndWidth / 2.f, wndHeight - 50};

    std::vector<Brick> bricks;

    constexpr int brkCountX{11};
    constexpr int brkCountY{4};
    constexpr int brkStartColumn{1};
    constexpr int brkStartRow{2};
    constexpr float brkSpacing{3};
    constexpr float brkOffsetX{22.f};

    for (int iX{0}; iX < brkCountX; ++iX) {
        for (int iY{0}; iY < brkCountY; ++iY) {
            float x{(iX + brkStartColumn) * (Brick::defWidth + brkSpacing)};
            float y{(iY + brkStartRow) * (Brick::defHeight + brkSpacing)};

            bricks.emplace_back(brkOffsetX + x, y);
        }
    }

    InitWindow(wndWidth, wndHeight, "Awesome Sauce");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        ball.update();
        paddle.update();
        for (auto& brick : bricks) {
            brick.update();
            solveBrickBallCollision(brick, ball);
        }
        
        bricks.erase(
            std::remove_if(std::begin(bricks), std::end(bricks),
            [](const auto& mBrick) { return mBrick.destroyed; }),
            std::end(bricks)
        );

        solvePaddleBallCollision(paddle, ball);

        BeginDrawing();
        ClearBackground(BLACK);
        ball.draw();
        paddle.draw();
        for (auto& brick : bricks) brick.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
