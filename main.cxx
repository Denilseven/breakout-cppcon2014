#include <algorithm>
#include <map>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <typeinfo>
#include <vector>

constexpr unsigned int wndWidth{800}, wndHeight{600};

class Entity {
public:
    bool destroyed{false};

    virtual ~Entity() {}
    virtual void update() {}
    virtual void draw() {}
};

class Manager {
private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::map<std::size_t, std::vector<Entity*>> groupedEntities;
    
public:
    // https://github.com/vittorioromeo/cppcon2014/blob/master/code/p10.cpp
    template <typename T, typename... TArgs>
    T& create(TArgs&&... mArgs) {
        static_assert(std::is_base_of<Entity, T>::value,
            "`T` must be derived from `Entity`");

        auto uPtr(std::make_unique<T>(std::forward<TArgs>(mArgs)...));
        auto ptr(uPtr.get());

        groupedEntities[typeid(T).hash_code()].emplace_back(ptr);
        entities.emplace_back(std::move(uPtr));

        return *ptr;
    }

    void refresh() {
        for (auto& pair: groupedEntities) {
            auto& vector(pair.second);

            vector.erase(
                std::remove_if(std::begin(vector), std::end(vector),
                [](auto mPtr) { return mPtr->destroyed; }),
                std::end(vector)
            );
        }
        entities.erase(
            std::remove_if(std::begin(entities), std::end(entities),
            [](const auto& mUPtr) { return mUPtr->destroyed; }),
            std::end(entities)
        );
    }

    void clear() {
        groupedEntities.clear();
        entities.clear();
    }

    template<typename T>
    auto& getAll() {
        return groupedEntities[typeid(T).hash_code()];
    }

    template<typename T, typename TFunc>
    void forEach(const TFunc& mFunc) {
        auto& vector(getAll<T>());

        for (auto ptr : vector) mFunc(*reinterpret_cast<T*>(ptr));
    }

    void update() {
        for (auto& e : entities)
            e->update();
    }

    void draw() {
        for (auto& e : entities)
            e->draw();
    }
};

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

class Ball : public Entity, public Circ {
public:
    static constexpr Color defColor{BLUE}; // "def" is for "default"
    static constexpr float defRadius{10.f};
    static constexpr float defVelocity{5.f};

    Vector2 velocity{-defVelocity, -defVelocity};

    Ball(float mX, float mY) {
        color = defColor;
        position = (Vector2){mX, mY};
        radius = defRadius;
    }

    Ball() : Ball(wndWidth / 2.f, wndHeight / 2.f) {};

    void update() override {
        position = Vector2Add(position, velocity);
        solveBoundCollisions();
    }

    void draw() override {
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

class Paddle : public Entity, public Rect {
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

    void update() override {
        processPlayerInput();
        position = Vector2Add(position, velocity);
    }

    void draw() override {
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

class Brick : public Entity, public Rect {
public:
    static constexpr Color defColor{RED};
    static constexpr float defWidth{60.f};
    static constexpr float defHeight{20.f};

    Brick(float mX, float mY) {
        color = defColor;
        position = (Vector2){mX, mY};
        width = defWidth;
        height = defHeight;
    }

    void update() override {}

    void draw() override {
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

    Manager manager;
    State state{State::InProgress};

public:
    Game() {
        InitWindow(wndWidth, wndHeight, "Awesome Sauce");
        SetTargetFPS(60);
    }

    void restart() {
        state = State::InProgress;

        manager.clear();

        manager.create<Ball>();
        manager.create<Paddle>();
        
        for (int iX{0}; iX < brkCountX; ++iX) {
            for (int iY{0}; iY < brkCountY; ++iY) {
                float x{(iX + brkStartColumn) * (Brick::defWidth + brkSpacing)};
                float y{(iY + brkStartRow) * (Brick::defHeight + brkSpacing)};

                manager.create<Brick>(brkOffsetX + x, y);
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
                manager.update();

                manager.forEach<Ball>([this](auto& mBall) {
                    manager.forEach<Brick>([this, &mBall](auto& mBrick) {
                        solveBrickBallCollision(mBrick, mBall);
                    });
                    manager.forEach<Paddle>([this, &mBall](auto& mPaddle) {
                        solvePaddleBallCollision(mPaddle, mBall);
                    });
                });

                manager.refresh();
            }

            BeginDrawing();
            ClearBackground(BLACK);
            manager.draw();
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
