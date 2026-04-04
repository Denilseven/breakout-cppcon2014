#include <algorithm>
#include <raylib.h>
#include <raymath.h>
#include <vector>

constexpr unsigned int wndWidth{800}, wndHeight{600};

struct Rect {
    Color color{GRAY};
    Vector2 position{0, 0};
    float width{0.f};
    float height{0.f};

    float x() const noexcept { return position.x; }
    float y() const noexcept { return position.y; }
    float left() const noexcept { return x() - (width / 2.f); }
    float right() const noexcept { return x() + (width / 2.f); }
    float top() const noexcept { return y() - (height / 2.f); }
    float bottom() const noexcept { return y() + (height / 2.f); }
};

struct Circ {
    Color color{GRAY};
    Vector2 position{0, 0};
    float radius{0.f};

    float x() const noexcept { return position.x; }
    float y() const noexcept { return position.y; }
    float left() const noexcept { return x() - radius; }
    float right() const noexcept { return x() + radius; }
    float top() const noexcept { return y() - radius; }
    float bottom() const noexcept { return y() + radius; }
};

class Ball : public Circ {
public:
    static constexpr Color defColor{BLUE}; // "def" is for "default"
    static constexpr float defRadius{10.f};
    static constexpr float defVelocity{3.f};

    Vector2 velocity{-defVelocity, -defVelocity};

    Ball(float mX, float mY) {
        color = defColor;
        position = (Vector2){mX, mY};
        radius = defRadius;
    }

    Ball() : Ball(wndWidth / 2.f, wndHeight / 2.f) {};

    void update() {
        position = Vector2Add(position, velocity);
        solveBoundCollisions();
    }

    void draw() {
        DrawCircle(position.x, position.y, radius, color);
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

class Paddle : public Rect {
public:
    static constexpr Color defColor{MAGENTA};
    static constexpr float defWidth{60.f};
    static constexpr float defHeight{20.f};
    static constexpr float defVelocity{8.f};

    Vector2 velocity{0, 0};

    Paddle(float mX, float mY) {
        color = defColor;
        position = (Vector2){mX, mY};
        width = defWidth;
        height = defHeight;
    }

    Paddle() : Paddle(wndWidth / 2.f, wndHeight - 50) {};

    void update() {
        processPlayerInput();
        position = Vector2Add(position, velocity);
    }

    void draw() {
        DrawRectanglePro(
            (Rectangle){position.x, position.y, width, height},
            (Vector2){width / 2.f, height / 2.f},
            0, color
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

class Brick : public Rect {
public:
    static constexpr Color defColor{RED};
    static constexpr float defWidth{60.f};
    static constexpr float defHeight{20.f};

    bool destroyed{false};

    Brick(float mX, float mY) {
        color = defColor;
        position = (Vector2){mX, mY};
        width = defWidth;
        height = defHeight;
    }

    void update() {}

    void draw() {
        DrawRectanglePro(
            (Rectangle){position.x, position.y, width, height},
            (Vector2){width / 2.f, height / 2.f},
            0, color
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

class Game {
private:
    enum class State {
        Paused,
        InProgress
    };

    static constexpr int brkCountX{11}, brkCountY{4};
    static constexpr int brkStartColumn{1}, brkStartRow{2};
    static constexpr float brkSpacing{3.f}, brkOffsetX{22.f};

    Ball ball;
    Paddle paddle;
    std::vector<Brick> bricks;

    State state{State::InProgress};

public:
    Game() {
        InitWindow(wndWidth, wndHeight, "Awesome Sauce");
        SetTargetFPS(60);
    }

    void restart() {
        state = State::InProgress;

        ball = Ball();
        paddle = Paddle();
        
        for (int iX{0}; iX < brkCountX; ++iX) {
            for (int iY{0}; iY < brkCountY; ++iY) {
                float x{(iX + brkStartColumn) * (Brick::defWidth + brkSpacing)};
                float y{(iY + brkStartRow) * (Brick::defHeight + brkSpacing)};

                bricks.emplace_back(brkOffsetX + x, y);
            }
        }
    }

    void run() {
        while (!WindowShouldClose()) {
            if (IsKeyPressed(KEY_P)) {
                if (state == State::Paused)
                    state = State::InProgress;
                else if (state == State::InProgress)
                    state = State::Paused;
            }

            if (IsKeyPressed(KEY_R)) {
                restart();
            }

            if (state == State::InProgress) {
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
            }

            BeginDrawing();
            ClearBackground(BLACK);
            ball.draw();
            paddle.draw();
            for (auto& brick : bricks) brick.draw();
            EndDrawing();
        }
    }
};

int main() {
    Game game;
    game.restart();
    game.run();
    CloseWindow();
    return 0;
}
