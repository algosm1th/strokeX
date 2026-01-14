#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

// Game has two states: start screen and playing
enum GameState {
    START_SCREEN,
    PLAYING
};

// Particle struct for drawing effects
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float lifetime;
    float maxLifetime;
    float size;
};

// A node is a circle point that players connect
struct Node {
    Vector2 position;
    int id;
    bool isHighlighted;
};

// An edge is a line connecting two nodes
struct Edge {
    int nodeA;
    int nodeB;
    int visitCount;
};

// Animated dot for start screen
struct AnimatedDot {
    Vector2 position;
    Vector2 velocity;
    float size;
    Color color;
    float speed;
};

// Main game class
class OneLinePuzzle {
private:
    vector<Node> nodes;
    vector<Edge> edges;
    vector<int> currentPath;
    vector<Vector2> pathPoints;
    vector<AnimatedDot> animatedDots;
    vector<Particle> particles;
    
    GameState gameState;
    bool isDrawing;
    bool levelComplete;
    int currentLevel;
    int maxUnlockedLevel;
    
    float nodeRadius;
    float levelStartTime;
    float levelEndTime;
    int currentScore;
    int totalScore;
    bool timerRunning;
    
    Vector2 lastParticleSpawnPos;
    float particleSpawnTimer;
    
    Rectangle startButton;
    Rectangle resetButton;
    Rectangle nextLevelButton;
    Rectangle prevLevelButton;
    Rectangle hintButton;
    
    bool showHintPopup;
    float hintPopupAlpha;
    bool hintPopupFadingIn;
    
    bool puzzleFailed;
    float shakeTimer;
    float shakeIntensity;
    Vector2 shakeOffset;
    
public:
    OneLinePuzzle() {
        gameState = START_SCREEN;
        isDrawing = false;
        levelComplete = false;
        currentLevel = 1;
        maxUnlockedLevel = 1;
        nodeRadius = 39.2f;
        levelStartTime = 0.0f;
        levelEndTime = 0.0f;
        currentScore = 0;
        totalScore = 0;
        timerRunning = false;
        particleSpawnTimer = 0.0f;
        lastParticleSpawnPos = {0, 0};
        showHintPopup = false;
        hintPopupAlpha = 0.0f;
        hintPopupFadingIn = false;
        puzzleFailed = false;
        shakeTimer = 0.0f;
        shakeIntensity = 0.0f;
        shakeOffset = {0, 0};
        
        startButton = {679, 471, 522, 131};
        resetButton = {1567, 177, 261, 92};
        nextLevelButton = {1567, 883, 261, 92};
        prevLevelButton = {1280, 883, 261, 92};
        hintButton = {1567, 295, 261, 92};
        
        InitializeAnimatedDots();
    }
    
    void InitializeAnimatedDots() {
        animatedDots.clear();
        for (int i = 0; i < 100; i++) {
            AnimatedDot dot;
            dot.position.x = (float)(GetRandomValue(0, 1880));
            dot.position.y = (float)(GetRandomValue(0, 1060));
            dot.size = 4.0f + (i % 5);
            dot.speed = 0.5f + (float)(i % 10) * 0.1f;
            
            float angle = (float)(i * 37) / 10.0f;
            dot.velocity.x = cosf(angle) * dot.speed;
            dot.velocity.y = sinf(angle) * dot.speed;
            
            switch(i % 4) {
                case 0: dot.color = Color{255, 0, 255, 180}; break;
                case 1: dot.color = Color{0, 255, 255, 180}; break;
                case 2: dot.color = Color{138, 43, 226, 180}; break;
                case 3: dot.color = Color{0, 255, 127, 180}; break;
            }
            
            animatedDots.push_back(dot);
        }
    }
    
    void UpdateAnimatedDots() {
        for (auto& dot : animatedDots) {
            dot.position.x += dot.velocity.x;
            dot.position.y += dot.velocity.y;
            
            if (dot.position.x <= 0 || dot.position.x >= 1880) {
                dot.velocity.x *= -1;
                dot.position.x = (dot.position.x <= 0) ? 0 : 1880;
            }
            if (dot.position.y <= 0 || dot.position.y >= 1060) {
                dot.velocity.y *= -1;
                dot.position.y = (dot.position.y <= 0) ? 0 : 1060;
            }
        }
    }
    
    int CountOddDegreeNodes() {
        vector<int> degrees(nodes.size(), 0);
        
        for (const auto& edge : edges) {
            degrees[edge.nodeA]++;
            degrees[edge.nodeB]++;
        }
        
        int oddCount = 0;
        for (int degree : degrees) {
            if (degree % 2 == 1) oddCount++;
        }
        
        return oddCount;
    }
    
    int GetFirstOddDegreeNode() {
        vector<int> degrees(nodes.size(), 0);
        
        for (const auto& edge : edges) {
            degrees[edge.nodeA]++;
            degrees[edge.nodeB]++;
        }
        
        for (size_t i = 0; i < degrees.size(); i++) {
            if (degrees[i] % 2 == 1) {
                return i;
            }
        }
        
        return 0; // If all even, can start anywhere
    }
    
    void TriggerShakeAnimation() {
        puzzleFailed = true;
        shakeTimer = 0.5f; // 0.5 second shake
        shakeIntensity = 10.0f;
    }
    
    void UpdateShakeAnimation(float deltaTime) {
        if (shakeTimer > 0) {
            shakeTimer -= deltaTime;
            
            // Generate random shake offset
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float currentIntensity = shakeIntensity * (shakeTimer / 0.5f);
            shakeOffset.x = cosf(angle) * currentIntensity;
            shakeOffset.y = sinf(angle) * currentIntensity;
            
            if (shakeTimer <= 0) {
                shakeOffset = {0, 0};
                puzzleFailed = false;
            }
        }
    }
    
    // Spawn particles along the drawing path
    void SpawnParticles(Vector2 position) {
        float distance = CalculateDistance(lastParticleSpawnPos, position);
        if (distance < 10.0f) return;
        
        lastParticleSpawnPos = position;
        
        for (int i = 0; i < 3; i++) {
            Particle p;
            p.position = position;
            
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float speed = (float)GetRandomValue(20, 60);
            p.velocity.x = cosf(angle) * speed;
            p.velocity.y = sinf(angle) * speed;
            
            p.maxLifetime = (float)GetRandomValue(30, 80) / 100.0f;
            p.lifetime = p.maxLifetime;
            p.size = (float)GetRandomValue(3, 7);
            
            int colorChoice = GetRandomValue(0, 3);
            switch(colorChoice) {
                case 0: p.color = Color{255, 0, 255, 255}; break;
                case 1: p.color = Color{138, 43, 226, 255}; break;
                case 2: p.color = Color{0, 255, 255, 255}; break;
                case 3: p.color = Color{255, 100, 255, 255}; break;
            }
            
            particles.push_back(p);
        }
    }
    
    void UpdateParticles(float deltaTime) {
        for (auto it = particles.begin(); it != particles.end();) {
            it->lifetime -= deltaTime;
            
            if (it->lifetime <= 0) {
                it = particles.erase(it);
            } else {
                it->position.x += it->velocity.x * deltaTime;
                it->position.y += it->velocity.y * deltaTime;
                
                it->velocity.x *= 0.95f;
                it->velocity.y *= 0.95f;
                
                float alpha = (it->lifetime / it->maxLifetime) * 255;
                it->color.a = (unsigned char)alpha;
                
                ++it;
            }
        }
    }
    
    void DrawParticles() {
        for (const auto& p : particles) {
            DrawCircleV(p.position, p.size, p.color);
        }
    }
    
    void DrawGlowText(const char* text, int x, int y, int fontSize, Color color) {
        for (int i = 3; i > 0; i--) {
            Color glowColor = color;
            glowColor.a = 50;
            DrawText(text, x - i, y, fontSize, glowColor);
            DrawText(text, x + i, y, fontSize, glowColor);
            DrawText(text, x, y - i, fontSize, glowColor);
            DrawText(text, x, y + i, fontSize, glowColor);
        }
        DrawText(text, x, y, fontSize, color);
    }
    
    void DrawGlowRect(Rectangle rect, Color color) {
        for (int i = 4; i > 0; i--) {
            Color glowColor = color;
            glowColor.a = 30;
            Rectangle glowRect = {rect.x - i, rect.y - i, rect.width + i*2, rect.height + i*2};
            DrawRectangleRounded(glowRect, 0.3f, 8, glowColor);
        }
        DrawRectangleRoundedLines(rect, 0.3f, 8, color);
    }
    
    // Enhanced button drawing with light background and glow on hover
    void DrawNeonButton(Rectangle button, const char* text, Color color, bool disabled = false, bool hovered = false) {
        Color btnColor = disabled ? Color{150, 150, 150, 255} : color;
        
        // Draw light background fill
        Color bgColor = btnColor;
        bgColor.a = hovered ? 60 : 30;
        DrawRectangleRounded(button, 0.3f, 8, bgColor);
        
        // Draw glow layers if hovered
        if (hovered && !disabled) {
            for (int i = 8; i > 0; i--) {
                Color glowColor = btnColor;
                glowColor.a = 20;
                Rectangle glowRect = {button.x - i, button.y - i, button.width + i*2, button.height + i*2};
                DrawRectangleRoundedLines(glowRect, 0.3f, 8, glowColor);
            }
        }
        
        // Draw outline
        DrawRectangleRoundedLines(button, 0.3f, 8, btnColor);
        
        // Draw text with glow if hovered
        int textWidth = MeasureText(text, 37);
        int textX = (int)button.x + ((int)button.width - textWidth) / 2;
        int textY = (int)button.y + 26;
        
        if (hovered && !disabled) {
            for (int i = 3; i > 0; i--) {
                Color glowColor = btnColor;
                glowColor.a = 50;
                DrawText(text, textX - i, textY, 37, glowColor);
                DrawText(text, textX + i, textY, 37, glowColor);
                DrawText(text, textX, textY - i, 37, glowColor);
                DrawText(text, textX, textY + i, 37, glowColor);
            }
        }
        DrawText(text, textX, textY, 37, btnColor);
    }
    
    void DrawHintPopup() {
        if (!showHintPopup && hintPopupAlpha <= 0) return;
        
        // Semi-transparent overlay
        Color overlayColor = Color{0, 0, 0, (unsigned char)(100 * hintPopupAlpha)};
        DrawRectangle(0, 0, 1880, 1060, overlayColor);
        
        // Popup box
        Rectangle popupBox = {540, 380, 800, 300};
        Color popupBg = Color{245, 245, 245, (unsigned char)(255 * hintPopupAlpha)};
        DrawRectangleRounded(popupBox, 0.2f, 10, popupBg);
        
        // Popup border with glow
        Color borderColor = Color{138, 43, 226, (unsigned char)(255 * hintPopupAlpha)};
        for (int i = 3; i > 0; i--) {
            Color glowColor = borderColor;
            glowColor.a = (unsigned char)(30 * hintPopupAlpha);
            Rectangle glowRect = {popupBox.x - i, popupBox.y - i, popupBox.width + i*2, popupBox.height + i*2};
            DrawRectangleRoundedLines(glowRect, 0.2f, 10, glowColor);
        }
        DrawRectangleRoundedLines(popupBox, 0.2f, 10, borderColor);
        
        // Title
        const char* title = "HINT";
        Color titleColor = Color{255, 0, 255, (unsigned char)(255 * hintPopupAlpha)};
        int titleWidth = MeasureText(title, 60);
        DrawText(title, 940 - titleWidth / 2, 410, 60, titleColor);
        
        // Hint text
        int oddCount = CountOddDegreeNodes();
        const char* hintText;
        Color hintColor = Color{50, 50, 50, (unsigned char)(255 * hintPopupAlpha)};
        
        if (oddCount == 0 || oddCount == 2) {
            if (oddCount == 2) {
                hintText = "Start from a node with odd connections!";
            } else {
                hintText = "You can start from any node!";
            }
        } else {
            hintText = "This puzzle has a solution - keep trying!";
        }
        
        int hintWidth = MeasureText(hintText, 36);
        DrawText(hintText, 940 - hintWidth / 2, 520, 36, hintColor);
        
        // Additional tip
        const char* tip = "Trace through each line exactly once.";
        Color tipColor = Color{100, 100, 100, (unsigned char)(255 * hintPopupAlpha)};
        int tipWidth = MeasureText(tip, 28);
        DrawText(tip, 940 - tipWidth / 2, 590, 28, tipColor);
        
        // Close instruction
        const char* closeText = "Click anywhere to close";
        Color closeColor = Color{138, 43, 226, (unsigned char)(200 * hintPopupAlpha)};
        int closeWidth = MeasureText(closeText, 24);
        DrawText(closeText, 940 - closeWidth / 2, 640, 24, closeColor);
    }
    
    void DrawStartScreen() {
        ClearBackground(WHITE);
        
        for (const auto& dot : animatedDots) {
            DrawCircleV(dot.position, dot.size, dot.color);
        }
        
        for (int i = 0; i < 6; i++) {
            Vector2 start = {196.0f, 141 + i * 143.0f};
            Vector2 end = {1684.0f, 188 + i * 143.0f};
            DrawLineEx(start, end, 3.3f, Color{138, 43, 226, 150});
        }
        
        const char* title = "StrokeX";
        int titleSize = 196;
        int titleX = 940 - MeasureText(title, titleSize) / 2;
        DrawGlowText(title, titleX, 165, titleSize, Color{255, 0, 255, 255});
        
        const char* subtitle = "One-Stroke Puzzle Challenge";
        int subSize = 42;
        int subX = 940 - MeasureText(subtitle, subSize) / 2;
        DrawText(subtitle, subX, 377, subSize, Color{138, 43, 226, 255});
        
        Vector2 mousePos = GetMousePosition();
        bool hovered = CheckCollisionPointRec(mousePos, startButton);
        
        Color buttonColor = hovered ? Color{255, 0, 255, 255} : Color{138, 43, 226, 255};
        DrawGlowRect(startButton, buttonColor);
        
        const char* buttonText = "START";
        int btnTextSize = 65;
        int btnTextX = (int)startButton.x + ((int)startButton.width - MeasureText(buttonText, btnTextSize)) / 2;
        DrawGlowText(buttonText, btnTextX, (int)startButton.y + 33, btnTextSize, buttonColor);
        
        const char* inst1 = "Draw through all lines once";
        const char* inst2 = "without lifting your finger!";
        DrawText(inst1, 940 - MeasureText(inst1, 36) / 2, 730, 36, Color{138, 43, 226, 255});
        DrawText(inst2, 940 - MeasureText(inst2, 36) / 2, 777, 36, Color{138, 43, 226, 255});
        
        Vector2 corners[4] = {{104, 94}, {1776, 94}, {1776, 966}, {104, 966}};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                float radius = 33 + j * 15;
                Color circleColor = Color{138, 43, 226, (unsigned char)(100 - j * 30)};
                DrawCircleLines((int)corners[i].x, (int)corners[i].y, radius, circleColor);
            }
        }
    }
    
    void LoadLevel(int level) {
        nodes.clear();
        edges.clear();
        currentPath.clear();
        pathPoints.clear();
        particles.clear();
        isDrawing = false;
        levelComplete = false;
        timerRunning = false;
        levelStartTime = 0.0f;
        levelEndTime = 0.0f;
        currentScore = 0;
        showHintPopup = false;
        hintPopupAlpha = 0.0f;
        puzzleFailed = false;
        shakeTimer = 0.0f;
        
        switch(level) {
            case 1:
                nodes = {
                    {{940, 441}, 0, false},
                    {{705, 707}, 1, false},
                    {{1175, 707}, 2, false}
                };
                edges = {
                    {0,1,0},{1,2,0},{2,0,0}
                };
                break;
                
            case 2:
                nodes = {
                    {{705,353},0,false}, {{940,353},1,false}, {{1175,353},2,false},
                    {{705,530},3,false}, {{940,530},4,false}, {{1175,530},5,false},
                    {{705,707},6,false}, {{940,707},7,false}, {{1175,707},8,false}
                };
                edges = {
                    {0,1,0}, {1,2,0}, {2,5,0}, {5,8,0},
                    {8,7,0}, {7,6,0}, {6,3,0}, {3,0,0},
                    {1,4,0}, {4,6,0}
                };
                break;
                
            case 3:
                nodes = {
                    {{940, 318}, 0, false}, {{1175, 495}, 1, false},
                    {{1081, 742}, 2, false}, {{799, 742}, 3, false},
                    {{705, 495}, 4, false}
                };
                edges = {
                    {0, 1, 0}, {1, 2, 0}, {2, 3, 0}, {3, 4, 0}, {4, 0, 0},
                    {0, 2, 0}, {1, 3, 0}, {2, 4, 0}, {3, 0, 0}, {4, 1, 0}
                };
                break;
                
            case 4:
                nodes = {
                    {{705, 441}, 0, false}, {{940, 441}, 1, false}, {{1175, 441}, 2, false},
                    {{705, 707}, 3, false}, {{940, 707}, 4, false}, {{1175, 707}, 5, false}
                };
                edges = {
                    {0, 1, 0}, {1, 2, 0}, {3, 4, 0}, {4, 5, 0},
                    {0, 3, 0}, {1, 4, 0}, {2, 5, 0}
                };
                break;
                
            case 5:
                nodes = {
                    {{588, 353}, 0, false}, {{822, 353}, 1, false},
                    {{1057, 353}, 2, false}, {{1292, 353}, 3, false},
                    {{705, 618}, 4, false}, {{940, 618}, 5, false},
                    {{1175, 618}, 6, false}
                };
                edges = {
                    {0, 1, 0}, {0, 4, 0}, {1, 2, 0}, {1, 4, 0},
                    {1, 5, 0}, {2, 3, 0}, {2, 5, 0}, {2, 6, 0},
                    {3, 6, 0}, {4, 5, 0}, {5, 6, 0}
                };
                break;
                
            case 6:
                nodes = {
                    {{940, 353}, 0, false}, {{822, 530}, 1, false},
                    {{1057, 530}, 2, false}, {{940, 795}, 3, false}
                };
                edges = {
                    {0, 1, 0}, {0, 2, 0}, {1, 2, 0}, {1, 3, 0}, {2, 3, 0}
                };
                break;
                
            case 7:
                nodes = {
                    {{940, 353}, 0, false}, {{705, 530}, 1, false},
                    {{1175, 530}, 2, false}, {{1175, 795}, 3, false},
                    {{705, 795}, 4, false}
                };
                edges = {
                    {0, 1, 0}, {0, 2, 0}, {1, 2, 0},
                    {1, 4, 0}, {2, 3, 0}, {3, 4, 0}
                };
                break;
                
            case 8:
                nodes = {
                    {{658, 318}, 0, false}, {{1128, 318}, 1, false},
                    {{1363, 565}, 2, false}, {{1128, 795}, 3, false},
                    {{658, 795}, 4, false}
                };
                edges = {
                    {0, 1, 0}, {0, 4, 0}, {1, 2, 0},
                    {1, 3, 0}, {1, 4, 0}, {2, 3, 0}, {3, 4, 0}
                };
                break;
                
            case 9:
                nodes = {
                    {{705, 353}, 0, false}, {{1175, 353}, 1, false},
                    {{1410, 618}, 2, false}, {{1175, 795}, 3, false},
                    {{705, 795}, 4, false}, {{470, 618}, 5, false}
                };
                edges = {
                    {0, 1, 0}, {0, 5, 0}, {1, 2, 0},
                    {1, 4, 0}, {2, 3, 0}, {3, 4, 0}, {4, 5, 0}
                };
                break;
                
            case 10:
                nodes = {
                    {{940,353},0,false},
                    {{822,530},1,false}, {{1057,530},2,false},
                    {{940,707},3,false},
                    {{1292,530},4,false}
                };
                edges = {
                    {0,1,0},{0,2,0},{1,3,0},{2,3,0},
                    {2,4,0}
                };
                break;
                
            case 11:
                nodes = {
                    {{705, 353}, 0, false}, {{822, 283}, 1, false},
                    {{940, 353}, 2, false}, {{940, 495}, 3, false},
                    {{822, 565}, 4, false}, {{705, 495}, 5, false},
                    {{1057, 283}, 6, false}, {{1175, 353}, 7, false},
                    {{1175, 495}, 8, false}, {{1057, 565}, 9, false}
                };
                edges = {
                    {0, 1, 0}, {1, 2, 0}, {2, 3, 0}, {3, 4, 0},
                    {4, 5, 0}, {5, 0, 0}, {2, 6, 0}, {6, 7, 0},
                    {7, 8, 0}, {8, 9, 0}, {9, 3, 0}
                };
                break;
                
            case 12:
                nodes = {
                    {{822,441},0,false}, {{705,618},1,false},
                    {{940,618},2,false}, {{1175,618},3,false}
                };
                edges = {
                    {0,1,0},{0,2,0},{1,2,0},
                    {2,3,0}
                };
                break;
                
            case 13:
                nodes = {
                    {{822,441},0,false}, {{1057,441},1,false},
                    {{940,530},2,false},
                    {{822,618},3,false}, {{1057,618},4,false}
                };
                edges = {
                    {0,1,0},{1,2,0},{2,0,0},
                    {2,3,0},{3,4,0},{4,2,0}
                };
                break;
                
            case 14:
                nodes = {
                    {{658, 389}, 0, false}, {{940, 389}, 1, false},
                    {{1222, 389}, 2, false}, {{658, 707}, 3, false},
                    {{1222, 707}, 4, false}
                };
                edges = {
                    {0, 1, 0}, {1, 2, 0}, {1, 4, 0}, {0, 3, 0}, {3, 4, 0}
                };
                break;
                
            default:
                currentLevel = 1;
                LoadLevel(1);
                break;
        }
    }
    
    bool AreNodesConnected(int nodeA, int nodeB) {
        for (const auto& edge : edges) {
            if ((edge.nodeA == nodeA && edge.nodeB == nodeB) ||
                (edge.nodeA == nodeB && edge.nodeB == nodeA)) {
                return true;
            }
        }
        return false;
    }
    
    void MarkEdgeVisited(int nodeA, int nodeB) {
        for (auto& edge : edges) {
            if ((edge.nodeA == nodeA && edge.nodeB == nodeB) ||
                (edge.nodeA == nodeB && edge.nodeB == nodeA)) {
                edge.visitCount++;
                return;
            }
        }
    }
    
    int GetEdgeVisitCount(int nodeA, int nodeB) {
        for (const auto& edge : edges) {
            if ((edge.nodeA == nodeA && edge.nodeB == nodeB) ||
                (edge.nodeA == nodeB && edge.nodeB == nodeA)) {
                return edge.visitCount;
            }
        }
        return 0;
    }
    
    float CalculateDistance(Vector2 a, Vector2 b) {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        return sqrtf(dx * dx + dy * dy);
    }
    
    int GetNodeAtPosition(Vector2 pos) {
        for (const auto& node : nodes) {
            float distance = CalculateDistance(pos, node.position);
            if (distance <= nodeRadius) {
                return node.id;
            }
        }
        return -1;
    }
    
    void StartPath(int nodeId) {
        ResetPath();
        currentPath.push_back(nodeId);
        pathPoints.push_back(nodes[nodeId].position);
        isDrawing = true;
        lastParticleSpawnPos = nodes[nodeId].position;
        
        if (!timerRunning) {
            levelStartTime = GetTime();
            timerRunning = true;
        }
    }
    
    void UpdatePath(Vector2 mousePos) {
        if (currentPath.empty()) return;
        
        int lastNode = currentPath.back();
        int nearestNode = GetNodeAtPosition(mousePos);
        
        if (nearestNode != -1 && nearestNode != lastNode) {
            if (AreNodesConnected(lastNode, nearestNode)) {
                currentPath.push_back(nearestNode);
                pathPoints.push_back(nodes[nearestNode].position);
                MarkEdgeVisited(lastNode, nearestNode);
            }
        }
    }
    
    void ResetPath() {
        currentPath.clear();
        pathPoints.clear();
        particles.clear();
        isDrawing = false;
        
        for (auto& edge : edges) {
            edge.visitCount = 0;
        }
        
        for (auto& node : nodes) {
            node.isHighlighted = false;
        }
    }
    
    int CalculateScore(float timeTaken) {
        int baseScore = 100;
        int timePenalty = (int)(timeTaken * 2);
        int score = max(20, baseScore - timePenalty);
        return score;
    }
    
    void CheckSolution() {
        bool allVisitedOnce = true;
        bool anyVisitedTwice = false;
        
        for (const auto& edge : edges) {
            if (edge.visitCount != 1) {
                allVisitedOnce = false;
            }
            if (edge.visitCount > 1) {
                anyVisitedTwice = true;
            }
        }
        
        if (allVisitedOnce) {
            levelComplete = true;
            levelEndTime = GetTime();
            float timeTaken = levelEndTime - levelStartTime;
            currentScore = CalculateScore(timeTaken);
            totalScore += currentScore;
            
            if (currentLevel == maxUnlockedLevel && currentLevel < 14) {
                maxUnlockedLevel = currentLevel + 1;
            }
        } else if (anyVisitedTwice) {
            // Puzzle failed - trigger shake
            TriggerShakeAnimation();
        }
    }
    
    float GetCurrentTime() {
        if (!timerRunning) return 0.0f;
        if (levelComplete) return levelEndTime - levelStartTime;
        return GetTime() - levelStartTime;
    }
    
    void Update() {
        Vector2 mousePos = GetMousePosition();
        float deltaTime = GetFrameTime();
        
        if (gameState == START_SCREEN) {
            UpdateAnimatedDots();
            
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mousePos, startButton)) {
                    gameState = PLAYING;
                    LoadLevel(currentLevel);
                }
            }
            return;
        }
        
        UpdateParticles(deltaTime);
        UpdateShakeAnimation(deltaTime);
        
        // Update hint popup fade
        if (showHintPopup && hintPopupFadingIn) {
            hintPopupAlpha += deltaTime * 4.0f;
            if (hintPopupAlpha >= 1.0f) {
                hintPopupAlpha = 1.0f;
                hintPopupFadingIn = false;
            }
        } else if (!showHintPopup && hintPopupAlpha > 0) {
            hintPopupAlpha -= deltaTime * 4.0f;
            if (hintPopupAlpha < 0) {
                hintPopupAlpha = 0;
            }
        }
        
        // Handle hint popup clicks
        if (showHintPopup && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            showHintPopup = false;
        }
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !showHintPopup) {
            if (CheckCollisionPointRec(mousePos, resetButton)) {
                ResetPath();
                return;
            }
            if (CheckCollisionPointRec(mousePos, hintButton)) {
                showHintPopup = true;
                hintPopupFadingIn = true;
                return;
            }
            if (CheckCollisionPointRec(mousePos, nextLevelButton)) {
                if (currentLevel < maxUnlockedLevel) {
                    currentLevel++;
                    LoadLevel(currentLevel);
                }
                return;
            }
            if (CheckCollisionPointRec(mousePos, prevLevelButton)) {
                if (currentLevel > 1) {
                    currentLevel--;
                    LoadLevel(currentLevel);
                }
                return;
            }
        }
        
        if (levelComplete || showHintPopup) return;
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int nodeId = GetNodeAtPosition(mousePos);
            if (nodeId != -1) {
                StartPath(nodeId);
            }
        }
        
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && isDrawing) {
            SpawnParticles(mousePos);
            UpdatePath(mousePos);
        }
        
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && isDrawing) {
            CheckSolution();
            isDrawing = false;
        }
        
        for (auto& node : nodes) {
            node.isHighlighted = false;
        }
        
        if (!levelComplete && !isDrawing) {
            int hoveredNode = GetNodeAtPosition(mousePos);
            if (hoveredNode != -1) {
                nodes[hoveredNode].isHighlighted = true;
            }
        }
    }
    
    void DrawGame() {
        ClearBackground(Color{245, 245, 245, 255});
        Vector2 mousePos = GetMousePosition();
        
        // Apply shake offset to all game elements
        Vector2 offset = shakeOffset;
        
        DrawText("StrokeX", 39 + (int)offset.x, 35 + (int)offset.y, 52, Color{255, 0, 255, 255});
        DrawText(TextFormat("Level: %d / %d", currentLevel, maxUnlockedLevel), 
                39 + (int)offset.x, 94 + (int)offset.y, 39, DARKGRAY);
        
        float currentTime = GetCurrentTime();
        Rectangle timerBox = {1567 + offset.x, 35 + offset.y, 261, 92};
        bool timerHovered = CheckCollisionPointRec(mousePos, timerBox);
        DrawNeonButton(timerBox, TextFormat("%.1fs", currentTime), Color{135, 60, 190, 255}, false, timerHovered);
        
        Rectangle shiftedResetBtn = {resetButton.x + offset.x, resetButton.y + offset.y, resetButton.width, resetButton.height};
        bool resetHovered = CheckCollisionPointRec(mousePos, shiftedResetBtn);
        DrawNeonButton(shiftedResetBtn, "RESET", Color{255, 100, 100, 255}, false, resetHovered);
        
        Rectangle shiftedHintBtn = {hintButton.x + offset.x, hintButton.y + offset.y, hintButton.width, hintButton.height};
        bool hintHovered = CheckCollisionPointRec(mousePos, shiftedHintBtn);
        DrawNeonButton(shiftedHintBtn, "HINT", Color{255, 200, 0, 255}, false, hintHovered);
        
        DrawText(TextFormat("Score: %d", totalScore), 39 + (int)offset.x, 153 + (int)offset.y, 36, DARKGRAY);
        
        for (const auto& edge : edges) {
            Vector2 start = {nodes[edge.nodeA].position.x + offset.x, nodes[edge.nodeA].position.y + offset.y};
            Vector2 end = {nodes[edge.nodeB].position.x + offset.x, nodes[edge.nodeB].position.y + offset.y};
            
            Color lineColor;
            float thickness = 6.5f;
            
            if (edge.visitCount == 0) {
                lineColor = Color{200, 200, 200, 255};
            } else if (edge.visitCount == 1) {
                lineColor = Color{100, 200, 100, 255};
                thickness = 9.8f;
            } else {
                lineColor = Color{255, 50, 50, 255};
                thickness = 13.1f;
            }
            
            DrawLineEx(start, end, thickness, lineColor);
        }
        
        if (pathPoints.size() > 1) {
            for (size_t i = 0; i < pathPoints.size() - 1; i++) {
                Vector2 p1 = {pathPoints[i].x + offset.x, pathPoints[i].y + offset.y};
                Vector2 p2 = {pathPoints[i + 1].x + offset.x, pathPoints[i + 1].y + offset.y};
                
                int nodeA = currentPath[i];
                int nodeB = currentPath[i + 1];
                int visitCount = GetEdgeVisitCount(nodeA, nodeB);
                
                Color pathColor = (visitCount > 1) ? 
                    Color{255, 50, 50, 255} : Color{138, 43, 226, 255};
                DrawLineEx(p1, p2, 13.1f, pathColor);
            }
        }
        
        if (isDrawing && !pathPoints.empty()) {
            Vector2 lastPoint = {pathPoints.back().x + offset.x, pathPoints.back().y + offset.y};
            DrawLineEx(lastPoint, mousePos, 9.8f, Color{138, 43, 226, 150});
        }
        
        DrawParticles();
        
        for (const auto& node : nodes) {
            Vector2 nodePos = {node.position.x + offset.x, node.position.y + offset.y};
            
            Color outerColor = node.isHighlighted ? 
                Color{138, 43, 226, 255} : Color{100, 100, 255, 255};
            DrawCircleV(nodePos, nodeRadius, outerColor);
            DrawCircleV(nodePos, nodeRadius - 6.5f, WHITE);
            
            if (find(currentPath.begin(), currentPath.end(), node.id) != currentPath.end()) {
                DrawCircleV(nodePos, nodeRadius - 13.1f, Color{138, 43, 226, 200});
            }
        }
        
        Rectangle shiftedPrevBtn = {prevLevelButton.x + offset.x, prevLevelButton.y + offset.y, prevLevelButton.width, prevLevelButton.height};
        bool prevHovered = CheckCollisionPointRec(mousePos, shiftedPrevBtn);
        DrawNeonButton(shiftedPrevBtn, "PREV", Color{100, 150, 255, 255}, false, prevHovered);

        Rectangle shiftedNextBtn = {nextLevelButton.x + offset.x, nextLevelButton.y + offset.y, nextLevelButton.width, nextLevelButton.height};
        bool nextHovered = CheckCollisionPointRec(mousePos, shiftedNextBtn);
        bool nextLevelLocked = (currentLevel >= maxUnlockedLevel);
        DrawNeonButton(shiftedNextBtn, "NEXT", Color{100, 200, 100, 255}, nextLevelLocked, nextHovered);
        if (nextLevelLocked && !levelComplete) {
            DrawText("LOCKED", (int)shiftedNextBtn.x + 46, (int)shiftedNextBtn.y + 98, 26, 
                    Color{150, 150, 150, 255});
        }
        
        if (levelComplete) {
            DrawRectangle(0, 0, 1880, 1060, Color{0, 0, 0, 150});
            DrawText("LEVEL COMPLETE!", 654, 389, 70, Color{100, 255, 100, 255});
            DrawText(TextFormat("+%d points!", currentScore), 823, 483, 52, Color{255, 215, 0, 255});
            DrawText(TextFormat("Time: %.1fs", levelEndTime - levelStartTime), 875, 553, 42, WHITE);
            DrawText("Press NEXT for next level", 693, 624, 42, WHITE);
        }
        
        DrawText("Draw through all lines once without lifting!", 39 + (int)offset.x, 977 + (int)offset.y, 32, DARKGRAY);
        
        // Draw hint popup on top of everything
        DrawHintPopup();
    }
    
    void Draw() {
        if (gameState == START_SCREEN) {
            DrawStartScreen();
        } else {
            DrawGame();
        }
    }
};

int main() {
    const int screenWidth = 1880;
    const int screenHeight = 1060;
    InitWindow(screenWidth, screenHeight, "STROKEX - One-Stroke Puzzle Game");
    SetTargetFPS(60);
    
    InitAudioDevice();
    Music backgroundMusic = LoadMusicStream("C:/Users/cW/Downloads/Cinema Sins Background Song (Clowning Around) - Background Music (HD).mp3");
    SetMusicVolume(backgroundMusic, 0.5f);
    PlayMusicStream(backgroundMusic);
    
    OneLinePuzzle game;
    
    while (!WindowShouldClose()) {
        UpdateMusicStream(backgroundMusic);
        game.Update();
        
        BeginDrawing();
        game.Draw();
        EndDrawing();
    }
    
    UnloadMusicStream(backgroundMusic);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}