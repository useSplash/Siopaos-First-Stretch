#include <raylib.h>
#include <raymath.h>
// #include "../raylib.h"
// #include "../raymath.h"

#include <iostream>

#include "entt.hpp"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float TARGET_FPS = 60;
const float TIMESTEP = 1 / TARGET_FPS;

// Game Constants
const float gravity = 500.0f;
const float drag = 1.0f;

// Stats
const float playerMoveSpeed = 20.0f;
const float playerSlingPower = 10.0f;

const float playerMaxHorizontalVelocity = 500.0f;
const float playerMaxVerticalVelocity = 800.0f;

const float playerAcceleration = 0.5f;
const float playerDeceleration = 10.0f;

const int numberOfPlatforms = 12;

int death_counter = 0;
int score = -1;
float lineThickness = 0.0f;

Vector2 initialSiopaoPos = {50,50};

Vector2 staticPlatformPos[numberOfPlatforms] = {
    {0,650}, // landing pad
    {120,700},
    {300,300},
    {230,610},
    {450,500},
    {690,350},
    {800,470},
    {990,500},
    {1000,360},
    {730,150},
    {880,200},
    {1060,80}
};

// Loop Variables
float accumulator = 0.0f;
Vector2 playerForces;
float frameTimer = 0.0f;

// ECS structs

struct PositionComponent {
    Vector2 position;
};
struct SizeComponent {
    int width;
    int height;
};
struct ColorComponent {
    Color color;
};
struct VelocityComponent {
    Vector2 velocity;
    float speed;
};
struct CircleColliderComponent {
    Vector2 center;
    int radius;
    bool onFloor;
};

struct PointComponent
{
    bool point;
};

////////////////////////
// UI structs and funcs
////////////////////////

// Generic UI component
struct UIComponent
{
    // Rectangle reprsenting the bounds of the UI component
    Rectangle bounds;

    // Draws this particular UI component
    // Set as abstract so that child widgets will implement this for us
    virtual void Draw() = 0;

    // Handles a mouse click event
    // Set as abstract so that child widgets will implement this for us
    // Returns a boolean indicating whether this UI component successfully handled the event
    virtual bool HandleClick(Vector2 click_position) = 0;
};

// Generic UI component that can contain other UI components as children
struct UIContainer : public UIComponent
{
    std::vector<UIComponent*> children;

    // Adds a child to the container
    void AddChild(UIComponent* child)
    {
        children.push_back(child);
    }

    // Adds a child to the container
    void RemoveChild(UIComponent* child)
    {
        for(int i = 0; i < children.size(); i++) {
            if(children[i] == child) {
                children.erase(children.begin()+i);
            }
        }
    }

    // Draw
    void Draw() override
    {
        // Since we are just a container for other widgets, we simply
        // call the draw function of other widgets.
        // This results in a pre-order traversal when we also draw child widgets that are also containers
        for (size_t i = 0; i < children.size(); ++i)
        {
            children[i]->Draw();
        }
    }

    // Handles a mouse click event
    // Returns a boolean indicating whether this UI component successfully handled the event
    bool HandleClick(Vector2 click_position) override
    {
        // Since we are just a container for other widgets, we call the HandleClick function of our child widgets
        // Since later children are drawn last, we do the opposite of draw where we start from the last child.
        // This results in a pre-order traversal but in the reverse order.
        for (size_t i = children.size(); i > 0; --i)
        {
            // If a child already handles the click event, we instantly return so no more child widgets handle the click
            if (children[i - 1]->HandleClick(click_position))
            {
                return true;
            }
        }

        return false;
    }
};

// Text display widget
struct Label : public UIComponent
{
    // Text to be displayed
    std::string text;

    // Draw
    void Draw() override
    {
        DrawText(text.c_str(), bounds.x, bounds.y, 14, BLACK);
    }

    // Handle mouse click
    // Returns a boolean indicating whether this UI component successfully handled the event
    bool HandleClick(Vector2 click_position) override
    {
        // Always return false since we're not going to handle click events for this particular widget
        // (unless you have to)
        return false;
    }
};

// Struct to encapsulate our UI library
struct UILibrary
{
    // Root container
    UIContainer root_container;

    // Updates the current UI state
    void Update()
    {
        // If the left mouse button was released, we handle the click from the root container
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            root_container.HandleClick(GetMousePosition());
        }
    }

    // Draw
    void Draw()
    {
        root_container.Draw();
    }
};

// Button widget
struct Button : public UIComponent
{
    // Text displayed by the button
    std::string text;
    UILibrary ui_library;
    Label label;
    Vector2 position;
    Vector2 (*func)(UILibrary ui_library, Label label, Vector2 position);

    // Draw
    void Draw() override
    {
        DrawRectangleRec(bounds, GRAY);
        DrawText(text.c_str(), bounds.x, bounds.y, 14, BLACK);
    }

    // Handle mouse click
    // Returns a boolean indicating whether this UI component successfully handled the event
    bool HandleClick(Vector2 click_position) override
    {
        // Check if the mouse click position is within our bounds
        if (CheckCollisionPointRec(click_position, bounds))
        {
            func(ui_library, label, position);
            return true;
        }

        return false;
    }
};


Vector2 GetClosestPointAABBCircle(Vector2 sioPos, Vector2 rectPos, Vector2 rectSize) {
    return {Clamp(sioPos.x, rectPos.x, rectPos.x + rectSize.x),
            Clamp(sioPos.y, rectPos.y, rectPos.y + rectSize.y)};
}


int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Siopao's First Stretch");
    SetTargetFPS(TARGET_FPS);

    UILibrary ui_library;
    ui_library.root_container.bounds = { 10, 10, 600, 500 };

    Label death_count;
    death_count.text = "Death Counter: " + std::to_string(death_counter);
    death_count.bounds = { 10, 10, 80, 40 };
    ui_library.root_container.AddChild(&death_count);

    Label score_text;
    score_text.text = "Score: " + std::to_string(score);
    score_text.bounds = { 150, 10, 80, 40 };
    ui_library.root_container.AddChild(&score_text);

    Label death_text;
    death_text.text = "You died! Try again!";
    death_text.bounds = { 10, 25, 80, 40 };
    // ui_library.root_container.AddChild(&death_text);

    Label victory_text;
    victory_text.text = "Yippee! Siopao made it to the steamer basket!";
    victory_text.bounds = { 10, 100, 80, 40 };
    // ui_library.root_container.AddChild(&victory_text);

    entt::registry registry;

    Texture2D siopao_texture = LoadTexture("assets/siopao spritesheet.png");
    Rectangle frameRec;
    frameRec.x = 0;
    frameRec.y = 0;
    frameRec.width = 64;
    frameRec.height = 64;

    Texture2D steamer_texture = LoadTexture("assets/steamer.png");
    Rectangle frameRecSteamer;
    frameRecSteamer.x = 0;
    frameRecSteamer.y = 0;
    frameRecSteamer.width = 64;
    frameRecSteamer.height = 48;
    Vector2 pos = {1080,40};
    // Vector2 pos = {50,600};


    float frameSelector = 0;

    //make siopao
    entt::entity siopao = registry.create();
    PositionComponent& pos_comp = registry.emplace<PositionComponent>(siopao);
    VelocityComponent& vel_comp = registry.emplace<VelocityComponent>(siopao);
    CircleColliderComponent& col_comp = registry.emplace<CircleColliderComponent>(siopao);
    pos_comp.position = {50,50};

    for (int i = 0; i < numberOfPlatforms; i++) {
        entt::entity platform = registry.create();
        PositionComponent& pos_comp = registry.emplace<PositionComponent>(platform);
        ColorComponent& color_comp = registry.emplace<ColorComponent>(platform);
        SizeComponent& size_comp = registry.emplace<SizeComponent>(platform);
        PointComponent& point_comp = registry.emplace<PointComponent>(platform);
        point_comp.point = false;
        pos_comp.position.x = staticPlatformPos[i].x;
        pos_comp.position.y = staticPlatformPos[i].y;
        color_comp.color = DARKBLUE;
        // pseudo random width for difficulty
        if (i == 0) {
            size_comp.width = 150;
        }
        else if (i % 2 == 0 && i != 0) {
            size_comp.width = 100/(i/2) + (i*20) - 20;
        } else if (i % 2 != 0 && i != numberOfPlatforms-1) {
            size_comp.width = i*30 - 30;
        } else {
            size_comp.width = 100;
        }
        size_comp.height = 25;
    }


    while (!WindowShouldClose()) {

        ui_library.Update();
        
        // Physics Loop
        float delta_time = GetFrameTime();
        accumulator += delta_time;

        //declate what we call for siopao
        auto Siopao = registry.view<PositionComponent, VelocityComponent>();
        auto Platform = registry.view<PositionComponent, ColorComponent, SizeComponent>();

        while (accumulator >= TIMESTEP) {

            // do stuff aka physics
            for (auto entity: Siopao) {
                PositionComponent& position = registry.get<PositionComponent>(entity);
                VelocityComponent& velocity = registry.get<VelocityComponent>(entity);
                CircleColliderComponent& collider = registry.get<CircleColliderComponent>(entity);

                // Player Info
                float playerBottomBound = position.position.y + frameRec.height;
                float playerLeftBound = position.position.x;
                float playerRightBound = position.position.x + frameRec.width;
                Vector2 playerCenterPos = {position.position.x + frameRec.width/2, position.position.y + frameRec.height/2};

                collider.onFloor = false;

                // Totaling every force done on the player at a given frame
                playerForces = {0, 0};

                if (frameTimer > 0.2f) {
                    frameSelector += 1;
                    if (frameSelector > 2) {
                        frameSelector = 0;
                    }
                    frameTimer = 0.0f;
                }

                if (abs(vel_comp.velocity.x) < 0.1f &&
                    abs(vel_comp.velocity.y) < 0.1f) {
                    frameRec.x = (64*frameSelector);
                }

                //For each platform
                for(auto entity: Platform)
                {
                    PositionComponent& rect_pos_comp = registry.get<PositionComponent>(entity);
                    SizeComponent& rect_size_comp = registry.get<SizeComponent>(entity);
                    PointComponent& rect_point_comp = registry.get<PointComponent>(entity);

                    //Clamp siopao to the platform
                    Vector2 closestPoint = GetClosestPointAABBCircle(Vector2Add(position.position, {frameRec.width/2,frameRec.height/2}), rect_pos_comp.position, {float (rect_size_comp.width), float (rect_size_comp.height)});
                    
                    float platformLeftBound = rect_pos_comp.position.x;
                    float platformRightBound = rect_pos_comp.position.x + rect_size_comp.width;
                    float platformUpperBound = rect_pos_comp.position.y;
                    Vector2 platformCenterPos = {rect_pos_comp.position.x + rect_size_comp.width/2, rect_pos_comp.position.y + rect_size_comp.height/2};

                    //if siopao is touching it
                    if (Vector2Distance(Vector2Add(position.position, {frameRec.width/2,frameRec.height/2}), closestPoint) <= frameRec.width/2) {
                        //do stuff  
                        if (playerBottomBound <= platformUpperBound + rect_size_comp.height/2) {
                            //position.position.y = rect_pos_comp.position.y - frameRec.height;
                            collider.onFloor = true;
                            if(!rect_point_comp.point)
                            {
                                rect_point_comp.point = true;
                                score += 1;
                                score_text.text = "Score: " + std::to_string(score);

                            }
                        }
                        else {
                            if (playerLeftBound > platformLeftBound && 
                                velocity.velocity.x < 0.0f) {

                                velocity.velocity.x = 5.0f;
                            }
                            if (playerRightBound < platformRightBound && 
                                velocity.velocity.x > 0.0f) {

                                velocity.velocity.x = -5.0f;
                            }
                        }
                    }

                }

                if((Vector2Distance(Vector2Add(position.position, {frameRec.width/2,frameRec.height/2}), Vector2Add(pos, {frameRecSteamer.width/2,frameRecSteamer.height/2})) <= frameRec.width/2)) {
                    score = 1000;
                    score_text.text = "Score: " + std::to_string(score);
                    ui_library.root_container.AddChild(&victory_text);
                }                    

                // Reaching Bottom Edge of Screen
                if (position.position.y + frameRec.height >= WINDOW_HEIGHT) {
                    position.position.y = WINDOW_HEIGHT - frameRec.height;
                    col_comp.onFloor = true;
                    ui_library.root_container.AddChild(&death_text);
                    position.position = initialSiopaoPos;
                    death_counter = death_counter + 1;
                    death_count.text = "Death Counter: " + std::to_string(death_counter);

                    score = -1;
                    score_text.text = "Score: " + std::to_string(score);
                }
                else {
                    //col_comp.onFloor = false;
                }
                if (position.position.y <= 0) {
                    if (vel_comp.velocity.y != 0.0f) {
                        vel_comp.velocity.y = 0.0f;   
                    }  
                    position.position.y = 0;
                }

                // Reaching Right Corner of Screen
                if (position.position.x + frameRec.width >= WINDOW_WIDTH) {
                    if (vel_comp.velocity.x > 0.0f) {
                        vel_comp.velocity.x = 0.0f;   
                    }  
                    position.position.x = WINDOW_WIDTH - frameRec.width;
                }

                // Reaching Left Corner of Screen
                if (position.position.x <= 0) {
                    if (vel_comp.velocity.x < 0.0f) {
                        vel_comp.velocity.x = 0.0f;   
                    }  
                    position.position.x = 0.0f;
                }

                if (col_comp.onFloor) {
                    //Basic Movement
                    if(IsKeyDown(KEY_A)) {
                        frameRec.x = (64*4);
                        //vel_comp.velocity = {-50,0};
                        playerForces = Vector2Add(playerForces, {-playerMoveSpeed, 0});
                        // vel_comp.speed = 1.5f;
                    }
                    if(IsKeyDown(KEY_D)) {
                        frameRec.x = (64*3);
                        //vel_comp.velocity = {50,0};
                        playerForces = Vector2Add(playerForces, {playerMoveSpeed, 0});
                        // vel_comp.speed = 1.5f;
                    }
                    
                    // Sling Mechanic
                    Vector2 initialMousePos;
                    Vector2 currentMousePos;
                    float force;
                    if (IsMouseButtonPressed(0)) {
                        initialMousePos = Vector2(GetMousePosition());
                        force = 0.0f;
                    }
                    if (initialMousePos.x != NULL && initialMousePos.y != NULL) {
                        while (IsMouseButtonDown(0)) {
                            currentMousePos = GetMousePosition();
                            force += TIMESTEP;
                            lineThickness += TIMESTEP;

                            frameSelector = 2;
                            
                            frameRec.x = (64*frameSelector);

                            break;
                        }
                    }
                    if (IsMouseButtonReleased(0)) {
                        playerForces = Vector2Add(playerForces,Vector2Scale(Vector2Negate(Vector2Subtract(currentMousePos, initialMousePos)),Clamp(force,0.0f,5.0f) * 2.0f));
                        lineThickness = 0.0f;
                    }
                }

                // Apply Player Forces
                vel_comp.velocity = Vector2Add(vel_comp.velocity, playerForces);

                if (!col_comp.onFloor) {
                    // Gravity
                    vel_comp.velocity = Vector2Add(vel_comp.velocity, {0.0f, gravity * TIMESTEP * 2});
                    vel_comp.velocity = Vector2Subtract(vel_comp.velocity, {vel_comp.velocity.x * drag * TIMESTEP * 2, 0.0f});

                    if (vel_comp.velocity.x < 0.0f) {
                        frameSelector = 4;
                        frameRec.x = (64*frameSelector);
                    }
                    if (vel_comp.velocity.x > 0.0f) {
                        frameSelector = 3;
                        frameRec.x = (64*frameSelector);
                    }
                }
                else {
                    // Stop on platform
                    if (vel_comp.velocity.y > 0.0f) {
                        vel_comp.velocity.y = 0.0f;   
                    } 
                    
                    if (vel_comp.velocity.y == 0.0f) {
                        // Deceleration Horizontal
                        vel_comp.velocity = Vector2Subtract(vel_comp.velocity, {vel_comp.velocity.x * playerDeceleration * TIMESTEP, 0.0f});
                    }
                }

                // Keep within Max Velocity
                vel_comp.velocity = {Clamp(vel_comp.velocity.x, -playerMaxHorizontalVelocity, playerMaxHorizontalVelocity), 
                                     Clamp(vel_comp.velocity.y, -playerMaxVerticalVelocity, playerMaxVerticalVelocity)};


                frameTimer += TIMESTEP;

                //just add velocity to position per timestep
                position.position = Vector2Add(position.position, Vector2Scale(velocity.velocity, TIMESTEP));
            }

            accumulator -= TIMESTEP;
        }
       

        BeginDrawing();
        ClearBackground(WHITE);
        DrawTextureRec(steamer_texture, frameRecSteamer, pos, WHITE);
        //based on position draw siopao
        for (auto entity: Siopao) {
            PositionComponent& position = registry.get<PositionComponent>(entity);
            DrawTextureRec(siopao_texture, frameRec, position.position, WHITE);
            if (IsMouseButtonDown(0)) {
                DrawLineEx({position.position.x+(frameRec.width/2), position.position.y+(frameRec.height/2)},GetMousePosition(),1.0f+lineThickness,RED);
            }
        }
        for (auto entity: Platform) {
            SizeComponent& size = registry.get<SizeComponent>(entity);
            PositionComponent& position = registry.get<PositionComponent>(entity);
            ColorComponent& color = registry.get<ColorComponent>(entity);

            DrawRectangle(position.position.x, position.position.y, size.width, size.height, color.color);
        }
        ui_library.Draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}