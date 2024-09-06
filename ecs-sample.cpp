#include <raylib.h>
#include <raymath.h>

#include <iostream>

#include "entt.hpp"

struct PositionComponent {
    Vector2 position;
};

struct CircleComponent {
    float radius;
};

struct RectangleComponent {
    float width;
    float height;
};

struct ColorComponent {
    Color color;
};

int main() {
    InitWindow(800, 600, "ECS");

    entt::registry registry;

    for (int x = 0; x < 5; x++) {
        entt::entity circle = registry.create();
        PositionComponent& pos_comp = registry.emplace<PositionComponent>(circle);
        pos_comp.position = {75.0f * float(x + 1), 100};
        CircleComponent& circ_comp = registry.emplace<CircleComponent>(circle);
        circ_comp.radius = 25.0f;
        ColorComponent& color_comp = registry.emplace<ColorComponent>(circle);
        color_comp.color = RED;
    }
    
    for (int x = 0; x < 5; x++) {
        entt::entity square = registry.create();
        PositionComponent& pos_comp = registry.emplace<PositionComponent>(square);
        pos_comp.position = {100, 75.0f * float(x + 1)};
        RectangleComponent& rect_comp = registry.emplace<RectangleComponent>(square);
        rect_comp.width = 50.0f;
        rect_comp.height = 50.0f;
        ColorComponent& color_comp = registry.emplace<ColorComponent>(square);
        color_comp.color = BLUE;
    }

    entt::entity square_circle = registry.create();
    PositionComponent& pos_comp = registry.emplace<PositionComponent>(square_circle);
    pos_comp.position = {100, 100};
    RectangleComponent& rect_comp = registry.emplace<RectangleComponent>(square_circle);
    rect_comp.width = 50.0f;
    rect_comp.height = 50.0f;
    CircleComponent& circ_comp = registry.emplace<CircleComponent>(square_circle);
    circ_comp.radius = 30.0f;
    ColorComponent& color_comp = registry.emplace<ColorComponent>(square_circle);
    color_comp.color = GREEN;
    
    while (!WindowShouldClose()) {
        auto move_circle = registry.view<CircleComponent, PositionComponent>();
        for (auto entity: move_circle) {
            PositionComponent& position = registry.get<PositionComponent>(entity);
            CircleComponent& circle = registry.get<CircleComponent>(entity);

            position.position.y += 100 * GetFrameTime();
        }
        auto move_rect = registry.view<RectangleComponent, PositionComponent>();
        for (auto entity: move_rect) {
            PositionComponent& position = registry.get<PositionComponent>(entity);
            RectangleComponent& rect = registry.get<RectangleComponent>(entity);

            position.position.x += 100 * GetFrameTime();
        }

        BeginDrawing();
        ClearBackground(WHITE);
        auto all_squares = registry.view<PositionComponent, RectangleComponent, ColorComponent>();
        for (auto entity: all_squares) {
            RectangleComponent& rectangle = registry.get<RectangleComponent>(entity);
            PositionComponent& position = registry.get<PositionComponent>(entity);
            ColorComponent& color = registry.get<ColorComponent>(entity);

            DrawRectangleV(position.position, {rectangle.width, rectangle.height}, color.color);
        }

        auto all_circles = registry.view<PositionComponent, CircleComponent, ColorComponent>();
        for (auto entity: all_circles) {
            CircleComponent& circle = registry.get<CircleComponent>(entity);
            PositionComponent& position = registry.get<PositionComponent>(entity);
            ColorComponent& color = registry.get<ColorComponent>(entity);

            DrawCircleV(position.position, circle.radius, color.color);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}