#include "Calc.h"
#include "framework.h"
#include "Mat4x4.h"

// Berlin Mini Jam, feb 2017

#define GFX_SX 800
#define GFX_SY 600

static bool insideTriangle(Vec2Arg a, Vec2Arg b, Vec2Arg c, Vec2Arg p);

int n1 = 1;
int n2 = 3;

static Mat4x4 transform;

enum TileType
{
    kTile_Normal,
    kTileType_Start,
    kTileType_End
};

enum TileState
{
    kTileState_Empty,
    kTileState_Idle,
    kTileState_Selected,
    kTileState_HoverDropOk,
    kTileState_HoverDropError,
};

struct Tile
{
    int numbers[3];
    bool active;
    TileType type;
    
    Tile()
    {
        memset(this, 0, sizeof(*this));
    }
    
    void setStart()
    {
        active = true;
        type = kTileType_Start;
    }
};

const float TILE_sx = 1.f;
const float TILE_sy = 0.866f;

static bool insideTile(float x, float y, const float * screenX, const float * screenY)
{
    Vec2 a(screenX[0], screenY[0]);
    Vec2 b(screenX[1], screenY[1]);
    Vec2 c(screenX[2], screenY[2]);
    Vec2 p(mouse.x, mouse.y);
    
    return insideTriangle(a, b, c, p);
}

#define MAP_SX 16
#define MAP_SY 16

#define MAP_SCALE 64.f

const float angle = Calc::DegToRad(30.f);

struct Map
{
    Tile tiles[MAP_SX][MAP_SY];
};

struct Bear
{
    float px;
    float py;
    float vx;
    float vy;
    
    float t;
    
    Bear()
    {
        memset(this, 0, sizeof(*this));
    }
    
    void tick(float dt)
    {
        if (t == 0.f)
        {
            t = 1.f;
            
            px = random(0, GFX_SX);
            py = random(0, GFX_SY);
            
            vx = random(-100, +100);
            vy = random(-200, 100);
        }
        
        px += vx * dt;
        py += vy * dt;
        
        vx += 50 * dt;
        vy += 100 * dt;
        
        t = Calc::Max(0.f, t - dt * .2f);
    }
    
    void draw()
    {
        setColor(colorWhite);
        Sprite s("bear.png");
        s.pivotX = s.getWidth()/2;
        s.pivotY = s.getHeight()/2;
        s.drawEx(px, py, std::sin(framework.time), .1f * std::sin(t * Calc::mPI));
    }
};

static Map map;

static Bear bear;

static int insideX = -1;
static int insideY = -1;

typedef void (IterateMapCallback)(const int x, const int y, Tile & tile, const bool flip, const float * screenX, const float * screenY);

static void iterateMap(IterateMapCallback cb)
{
    for (int x = 0; x < MAP_SX; ++x)
    {
        for (int y = 0; y < MAP_SY; ++y)
        {
            Tile & tile = map.tiles[x][y];
            
            const int index = x + y;
            const bool flip = (index & 1) == 1;
            
            float tx = x / 2.f;
            float ty = y * TILE_sy;

            float x1 = 0.f;
            float y1 = -TILE_sy/2;
            float x2 = -TILE_sx/2;
            float y2 = +TILE_sy/2;
            float x3 = +TILE_sx/2;
            float y3 = +TILE_sy/2;

            if (flip)
            {
                y1 = -y1;
                y2 = -y2;
                y3 = -y3;
            }

            x1 += tx;
            y1 += ty;
            x2 += tx;
            y2 += ty;
            x3 += tx;
            y3 += ty;
            
            const Vec2 v1 = transform * Vec2(x1, y1);
            const Vec2 v2 = transform * Vec2(x2, y2);
            const Vec2 v3 = transform * Vec2(x3, y3);
            
            float screenX[3] = { v1[0], v2[0], v3[0] };
            float screenY[3] = { v1[1], v2[1], v3[1] };
            
            cb(x, y, tile, flip, screenX, screenY);
        }
    }
}

static bool isNeighbor(const int x, const int y, const bool flip, const int px, const int py)
{
    if (y == py && (x == px - 1 || x == px + 1))
        return true;
    else if (x != px)
        return false;
    else if (flip && y == py + 1)
        return true;
    else if (!flip && y == py - 1)
        return true;
    else
        return false;
}

static Tile * getTile(int x, int y)
{
    if (x >= 0 && x < MAP_SX && y >= 0 && y < MAP_SY)
        return &map.tiles[x][y];
    else
        return nullptr;
}

static bool checkNeighbors(int x, int y, Tile * m)
{
    const int index = x + y;
    const bool flip = (index & 1) == 1;
    
    bool v = true;
    
    if (!flip)
    {
        Tile * l = getTile(x - 1, y);
        Tile * r = getTile(x + 1, y);
        Tile * t = getTile(x, y + 1);
        
        int numActive = 0;
        
        if (l && l->active)
            numActive++;
        if (r && r->active)
            numActive++;
        if (t && t->active)
            numActive++;
        
        v &= numActive > 0;
        
#if 0
        if (l)
        {
            v &= m->numbers[0] == l->numbers[2];
            v &= m->numbers[1] == l->numbers[0];
        }
        
        if (r)
        {
            v &= m->numbers[0] == r->numbers[1];
            v &= m->numbers[2] == r->numbers[0];
        }
        
        if (t)
        {
            v &= m->numbers[2] == t->numbers[2];
            v &= m->numbers[1] == t->numbers[1];
        }
#endif
    }
    else
    {
        Tile * l = getTile(x - 1, y);
        Tile * r = getTile(x + 1, y);
        Tile * t = getTile(x, y - 1);
        
        int numActive = 0;
        
        if (l && l->active)
            numActive++;
        if (r && r->active)
            numActive++;
        if (t && t->active)
            numActive++;
        
        v &= numActive > 0;
        
#if 0
        if (l)
        {
            v &= m->numbers[0] == l->numbers[2];
            v &= m->numbers[1] == l->numbers[0];
        }
        
        if (r)
        {
            v &= m->numbers[2] == r->numbers[0];
            v &= m->numbers[0] == r->numbers[1];
        }
        
        if (t)
        {
            v &= m->numbers[1] == t->numbers[1];
            v &= m->numbers[2] == t->numbers[2];
        }
#endif
    }
    
    return v;
}

static void drawTile(const int x, const int y, Tile & tile, const bool flip, TileState state, const float * screenX, const float * screenY)
{
    if (tile.type == kTileType_Start)
        setColor(colorRed);
    else if (tile.type == kTileType_End)
        setColor(colorRed);
    else if (state == kTileState_Empty)
        setColorf(.2f, .2f, .2f, 1.f);
    else if (state == kTileState_Idle)
    {
        if (flip)
            setColorf(.5f, .5f, .5f, 1.f);
        else
            setColorf(.6f, .6f, .6f, 1.f);
    }
    else if (state == kTileState_Selected)
        setColorf(1.f, 0.f, 0.f, 1.f);
    else if (state == kTileState_HoverDropOk)
        setColorf(1.f, 1.f, 0.f, 1.f);
    else if (state == kTileState_HoverDropError)
        setColorf(.5f, .5f, 0.f, 1.f);
    else
        setColorf(0.f, 1.f, 0.f, 1.f);
    
    gxBegin(GL_TRIANGLES);
    {
        for (int i = 0; i < 3; ++i)
            gxVertex2f(screenX[i], screenY[i]);
    }
    gxEnd();
    
    if (state != kTileState_Empty)
    {
        float midX = (screenX[0] + screenX[1] + screenX[2]) / 3.f;
        float midY = (screenY[0] + screenY[1] + screenY[2]) / 3.f;
        for (int i = 0; i < 3; ++i)
        {
            float x = screenX[i];
            float y = screenY[i];
            float dx = x - midX;
            float dy = y - midY;
            float ds = std::sqrt(dx * dx + dy * dy);
            float nx = dx / ds;
            float ny = dy / ds;
            float dd = 16;
            x -= nx * dd;
            y -= ny * dd;
            
            setColor(colorWhite);
            drawText(x, y, 12, 0.f, -1.f, "%d", tile.numbers[i]);
            //drawText(x, y, 12, 0.f, -1.f, "%d", i + 1);
        }
    }
    
    gxBegin(GL_LINE_LOOP);
    {
        gxColor4f(1.f, 1.f, 1.f, .2f);
        for (int i = 0; i < 3; ++i)
        {
            gxVertex2f(screenX[i], screenY[i]);
        }
    }
    gxEnd();
}

struct Parasite
{
    bool active;
    
    float m;
    float s;
    int sx;
    int sy;
    int dx;
    int dy;
    
    Parasite()
    {
        memset(this, 0, sizeof(*this));
        
        s = random(.7f, 1.f) * 3.f;
    }
    
    void tick(const float dt)
    {
        m = Calc::Min(1.f, m + dt * s);
        
        if (m == 1.f)
        {
            for (int r = 0; r < 100; ++r)
            {
                int nx = dx;
                int ny = dy;
                
                const int index = dx + dy;
                const bool flip = (index & 1) == 1;
                const bool hori = rand() % 2;
                
                if (hori)
                {
                    if (rand() % 2)
                        nx = dx - 1;
                    else
                        nx = dx + 1;
                }
                else
                {
                    if (flip)
                        ny = dy - 1;
                    else
                        ny = dy + 1;
                }
                
                if (nx >= 0 && nx < MAP_SX && ny >= 0 && ny < MAP_SY)
                {
                    if (map.tiles[nx][ny].active)
                    {
                        sx = dx;
                        sy = dy;
                        dx = nx;
                        dy = ny;
                        
                        m = 0.f;
                        
                        Sound("token-drop.ogg").play();
                        
                        break;
                    }
                }
            }
        }
    }
    
    void draw()
    {
        gxPushMatrix();
        {
            float x = Calc::Lerp(sx, dx, m);
            float y = Calc::Lerp(sy, dy, m);
            
            float tx = x / 2.f;
            float ty = y * TILE_sy;
            
            gxTranslatef(tx, ty, 0.f);
            
            setColor(colorWhite);
            
            fillCircle(0.f, 0.f, Calc::Lerp(.2f, .15f, (std::sinf(framework.time * 2.f) + 1.f) / 2.f), 20);
        }
        gxPopMatrix();
    }
};

#define NUMPARA 10

static Parasite para[NUMPARA];

#define PILE_S 5

typedef void (IteratePileCallback)(const int index, Tile & tile, const float * screenX, const float * screenY);

static void iteratePile(Tile * tiles, const int numTiles, IteratePileCallback cb)
{
    for (int i = 0; i < numTiles; ++i)
    {
        float x = 50;
        float y = GFX_SY - 350 + i * 60;
        float scale = MAP_SCALE;
        
        float sx = TILE_sx * scale;
        float sy = TILE_sy * scale;
        
        float screenX[3];
        float screenY[3];
        
        screenX[0] = x;
        screenY[0] = y - sy/2;
        screenX[1] = x - sx/2;
        screenY[1] = y + sy/2;
        screenX[2] = x + sx/2;
        screenY[2] = y + sy/2;
        
        cb(i, tiles[i], screenX, screenY);
    }
}

struct Pile
{
    Tile tiles[PILE_S];
    
    int selectIndex;
    int hoverIndex;
    
    float selectX;
    float selectY;
    
    Pile()
    {
        selectIndex = -1;
        hoverIndex = -1;
    }
    
    void tick();
    void draw();
    
    void addToPile(int n1, int n2, int n3)
    {
        for (int i = 0; i < PILE_S; ++i)
        {
            if (!tiles[i].active)
            {
                tiles[i].active = true;
                tiles[i].numbers[0] = n1;
                tiles[i].numbers[1] = n2;
                tiles[i].numbers[2] = n3;
                return;
            }
        }
    }
};

static Pile pile;

static void iteratePileTick(const int index, Tile & tile, const float * screenX, const float * screenY)
{
    if (!tile.active)
        return;
    
    if (insideTile(mouse.x, mouse.y, screenX, screenY))
        pile.hoverIndex = index;
}

void Pile::tick()
{
    hoverIndex = -1;
    
    iteratePile(tiles, PILE_S, iteratePileTick);
    
    if (mouse.wentDown(BUTTON_LEFT))
    {
        if (hoverIndex != -1)
        {
            selectIndex = hoverIndex;
            selectX = mouse.x;
            selectY = mouse.y;
        }
    }
    
    if (mouse.wentUp(BUTTON_LEFT))
    {
        if (selectIndex != -1)
        {
            if (insideX != -1 && insideY != -1)
            {
                if (checkNeighbors(insideX, insideY, &tiles[selectIndex]))
                {
                    map.tiles[insideX][insideY] = tiles[selectIndex];
                
                    tiles[selectIndex].active = false;
                }
            }
        }
        
        selectIndex = -1;
    }
    
    //logDebug("hover: %d, select:%d", hoverIndex, selectIndex);
    
    bool empty = true;
    
    for (int i = 0; i < PILE_S; ++i)
        if (tiles[i].active)
            empty = false;
    
    if (empty)
    {
        for (int i = 0; i < PILE_S; ++i)
        {
            pile.addToPile(random(n1, n2), random(n1, n2), random(n1, n2));
        }
    }
}

static void iteratePileDraw(const int index, Tile & tile, const float * screenX, const float * screenY)
{
    if (!tile.active)
        return;
    
    bool selected = (index == pile.selectIndex);
    bool hover = (index == pile.hoverIndex);
    
    float dx = 0.f;
    float dy = 0.f;
    
    bool canPlace = false;
    
    if (selected)
    {
        dx = mouse.x - pile.selectX;
        dy = mouse.y - pile.selectY;
        
        if (insideX != -1)
        {
            canPlace = checkNeighbors(insideX, insideY, &tile);
            //canPlace = true;
        }
    }
    
    gxPushMatrix();
    {
        gxTranslatef(dx, dy, 0.f);
        
        TileState state =
            selected ? kTileState_Selected :
            hover ? kTileState_HoverDropOk :
            kTileState_Idle;
        
        drawTile(0, 0, tile, false, state, screenX, screenY);
    }
    gxPopMatrix();
}

void Pile::draw()
{
    iteratePile(tiles, PILE_S, iteratePileDraw);
}

static bool insideTriangle(Vec2Arg a, Vec2Arg b, Vec2Arg c, Vec2Arg p)
{
    // Compute vectors
    Vec2 v0 = c - a;
    Vec2 v1 = b - a;
    Vec2 v2 = p - a;
    
    // Compute dot products
    float dot00 = v0 * v0;
    float dot01 = v0 * v1;
    float dot02 = v0 * v2;
    float dot11 = v1 * v1;
    float dot12 = v1 * v2;
    
    // Compute barycentric coordinates
    float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    
    // Check if point is in triangle
    return (u >= 0.f) && (v >= 0.f) && (u + v < 1.f);
}

static void randomize()
{
    map = Map();
    
    for (int x = 0; x < MAP_SX; ++x)
    {
        for (int y = 0; y < MAP_SY; ++y)
        {
            for (int i = 0; i < 3; ++i)
            {
                map.tiles[x][y].active = false;
                
                map.tiles[x][y].numbers[i] = random(n1, n2);
            }
        }
    }
    
    for (int i = 0; i < NUMPARA; ++i)
    {
        const int x = rand() % MAP_SX;
        const int y = rand() % MAP_SY;
        
        map.tiles[x][y].setStart();
        
        para[i] = Parasite();
        
        para[i].active = true;
        para[i].sx = x;
        para[i].sy = y;
        para[i].dx = x;
        para[i].dy = y;
    }
}

int main(int argc, char * argv[])
{
    changeDirectory("data");
    
    //framework.fullscreen = true;
    
    if (framework.init(0, nullptr, GFX_SX, GFX_SY))
    {
        Music("song.ogg").play();
        
        Surface surface(GFX_SX, GFX_SY, false);
        
        randomize();
        
        float csx = 0.f;
        float csy = 0.f;
        float cdx = 0.f;
        float cdy = 0.f;
        float zs = 1.f;
        float zd = 1.f;
        
        float ft = 0.f;
        float kt = 0.f;
        
        while (!framework.quitRequested)
        {
            framework.process();
            
            if (keyboard.wentDown(SDLK_ESCAPE))
                framework.quitRequested = true;
            
            const float dt = framework.timeStep;
            
            if (keyboard.wentDown(SDLK_p))
                randomize();
            
            if (keyboard.isDown(SDLK_LEFT))
                cdx -= 300.f * dt;
            if (keyboard.isDown(SDLK_RIGHT))
                cdx += 300.f * dt;
            if (keyboard.isDown(SDLK_UP))
                cdy -= 300.f * dt;
            if (keyboard.isDown(SDLK_DOWN))
                cdy += 300.f * dt;
            if (keyboard.wentDown(SDLK_a))
                zd *= 2.f;
            if (keyboard.wentDown(SDLK_z))
                zd /= 2.f;
            
            if (keyboard.wentDown(SDLK_SPACE))
            {
                Sound("token-pickup.ogg").play();
                
                ft = 1.f;
                kt = 1.f;
            }
            
            const float f = std::pow(.2f, dt);
            csx = Calc::Lerp(cdx, csx, f);
            csy = Calc::Lerp(cdy, csy, f);
            
            zs = Calc::Lerp(zd, zs, f);
            
            ft = Calc::Max(0.f, ft - dt * 2.f);
            kt = Calc::Max(0.f, kt - dt * .5f);
            
            transform = Mat4x4(true).Translate(GFX_SX/2, GFX_SY/2, 0).Scale(zs, zs, 1.f).Translate(-csx, -csy, 0.f).RotateZ(angle).Scale(MAP_SCALE, MAP_SCALE, 1.f).Translate(-MAP_SX/2/2, -MAP_SY/2 * TILE_sy, 0);
            
            framework.beginDraw(0, 0, 0, 0);
            {
                pushSurface(&surface);
                {
                    setBlend(BLEND_ALPHA);
                    
                    setColorf(0, 0, 0, .05);
                    drawRect(0, 0, GFX_SX, GFX_SY);
                    
                    insideX = -1;
                    insideY = -1;
                    
                    iterateMap(
                               [](const int x, const int y, Tile & tile, const bool flip, const float * screenX, const float * screenY)
                               {
                                   if (insideTile(mouse.x, mouse.y, screenX, screenY))
                                   {
                                       insideX = x;
                                       insideY = y;
                                   }
                               });
                    
                    for (int i = 0; i < NUMPARA; ++i)
                        para[i].tick(dt);
                    
                    pile.tick();
                    
                    bear.tick(dt);
                    
                    setFont("calibri.ttf");
                    
                    iterateMap(
                        [](const int x, const int y, Tile & tile, const bool flip, const float * screenX, const float * screenY)
                        {
                            TileState state;
                            
                            if (pile.selectIndex != -1 && x == insideX && y == insideY)
                            {
                                if (checkNeighbors(insideX, insideY, &pile.tiles[pile.selectIndex]))
                                    state = kTileState_HoverDropOk;
                                else
                                    state = kTileState_HoverDropError;
                                                   
                            }
                            else if (!tile.active)
                                state = kTileState_Empty;
                                //state = kTileState_Idle;
                            else
                                state = kTileState_Idle;
                            
                            //logDebug("sx=%d, sy=%d", insideX, insideY);
                            
                            drawTile(x, y, tile, flip, state, screenX, screenY);
                        });
                    
                    gxPushMatrix();
                    {
                        gxMultMatrixf(transform.m_v);
                        
                        for (int i = 0; i < NUMPARA; ++i)
                            para[i].draw();
                    }
                    gxPopMatrix();
                    
                    pile.draw();
                    
                    gxColor4f(1.f, 1.f, 1.f, ft);
                    fillCircle(GFX_SX/2, GFX_SY/2, ft * GFX_SY, 100);
                }
                popSurface();
                
                Shader shader("kaleido");
                setShader(shader);
                shader.setImmediate("time", (std::sin(framework.time) + 1) / 2.f);
                shader.setTexture("colormap", 0, surface.getTexture());
                shader.setImmediate("alpha", kt * .8f);
                surface.postprocess();
                clearShader();
                
                gxSetTexture(surface.getTexture());
                {
                    setBlend(BLEND_OPAQUE);
                    setColor(colorWhite);
                    drawRect(0, 0, GFX_SX, GFX_SY);
                }
                gxSetTexture(0);
                
                setBlend(BLEND_ALPHA);
                drawText(20, 10, 32, +1, +1, "#proudtobebear");
                
                bear.draw();
            }
            framework.endDraw();
        }
        
        framework.shutdown();
    }
    
    return 0;
}
