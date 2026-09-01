/*
  ================================================================
  R2D2 COMPLEX MKIV
  Advanced Astromech Diagnostic Display

  ESP32-C3
  1.28" 240x240 GC9A01 round display
  LovyanGFX

  Design:
  - Blue / cyan R2-D2 inspired interface
  - No green
  - Slow, smooth animations
  - No interaction required
  - No WiFi required
  - No sound
  - Multiple diagnostic animation scenes
  - Text and animation areas are separated
  ================================================================
*/

#define LGFX_USE_V1

#include <Arduino.h>
#include <LovyanGFX.hpp>


// ================================================================
// DISPLAY CONFIGURATION
// ================================================================

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Bus_SPI _bus;
  lgfx::Panel_GC9A01 _panel;

public:

  LGFX()
  {
    auto bus = _bus.config();

    bus.spi_host = SPI2_HOST;
    bus.spi_mode = 0;

    bus.freq_write = 80000000;
    bus.freq_read  = 20000000;

    bus.spi_3wire = false;
    bus.use_lock = true;
    bus.dma_channel = SPI_DMA_CH_AUTO;

    bus.pin_sclk = 6;
    bus.pin_mosi = 7;
    bus.pin_miso = -1;
    bus.pin_dc   = 2;

    _bus.config(bus);
    _panel.setBus(&_bus);


    auto panel = _panel.config();

    panel.pin_cs = 10;
    panel.pin_rst = -1;
    panel.pin_busy = -1;

    panel.panel_width = 240;
    panel.panel_height = 240;

    panel.memory_width = 240;
    panel.memory_height = 240;

    panel.offset_x = 0;
    panel.offset_y = 0;
    panel.offset_rotation = 0;

    panel.dummy_read_pixel = 8;
    panel.dummy_read_bits = 1;

    panel.readable = false;
    panel.invert = true;
    panel.rgb_order = false;
    panel.dlen_16bit = false;
    panel.bus_shared = false;

    _panel.config(panel);

    setPanel(&_panel);
  }
};


LGFX lcd;
LGFX_Sprite canvas(&lcd);


// ================================================================
// CONSTANTS
// ================================================================

constexpr int W = 240;
constexpr int H = 240;

constexpr uint32_t FRAME_TIME = 33;


// ================================================================
// COLOR PALETTE
// ================================================================

constexpr uint16_t BLACK      = 0x0000;

constexpr uint16_t NAVY       = 0x0009;
constexpr uint16_t BLUE_DARK  = 0x0018;
constexpr uint16_t BLUE       = 0x035F;
constexpr uint16_t BLUE_LIGHT = 0x04BF;
constexpr uint16_t CYAN       = 0x07FF;

constexpr uint16_t WHITE_BLUE = 0xDFFF;
constexpr uint16_t WHITE      = 0xFFFF;


// ================================================================
// SYSTEM SCENES
// ================================================================

enum Scene
{
  SCENE_CORE,
  SCENE_OPTICS,
  SCENE_GYRO,
  SCENE_RADAR,
  SCENE_POWER,
  SCENE_MEMORY,
  SCENE_SERVO,
  SCENE_SIGNAL,
  SCENE_HOLOGRAM,
  SCENE_NAVIGATION
};


Scene scene = SCENE_CORE;

uint32_t sceneStart = 0;
uint32_t sceneLength = 10000;
uint32_t lastFrame = 0;


// ================================================================
// BASIC MATH
// ================================================================

float rad(float degrees)
{
  return degrees * PI / 180.0f;
}


float px(
  float cx,
  float radius,
  float angle)
{
  return cx + cosf(rad(angle)) * radius;
}


float py(
  float cy,
  float radius,
  float angle)
{
  return cy + sinf(rad(angle)) * radius;
}


float sceneTime()
{
  return (millis() - sceneStart) / 1000.0f;
}


// ================================================================
// STATE CONTROL
// ================================================================

void setScene(
  Scene newScene,
  uint32_t duration)
{
  scene = newScene;
  sceneStart = millis();
  sceneLength = duration;
}


void nextScene()
{
  switch (scene)
  {
    case SCENE_CORE:
      setScene(SCENE_OPTICS, 9500);
      break;

    case SCENE_OPTICS:
      setScene(SCENE_GYRO, 10500);
      break;

    case SCENE_GYRO:
      setScene(SCENE_RADAR, 9500);
      break;

    case SCENE_RADAR:
      setScene(SCENE_POWER, 9000);
      break;

    case SCENE_POWER:
      setScene(SCENE_MEMORY, 10000);
      break;

    case SCENE_MEMORY:
      setScene(SCENE_SERVO, 9000);
      break;

    case SCENE_SERVO:
      setScene(SCENE_SIGNAL, 9500);
      break;

    case SCENE_SIGNAL:
      setScene(SCENE_HOLOGRAM, 9000);
      break;

    case SCENE_HOLOGRAM:
      setScene(SCENE_NAVIGATION, 10500);
      break;

    default:
      setScene(SCENE_CORE, 12000);
      break;
  }
}


// ================================================================
// DRAWING HELPERS
// ================================================================

void circle(
  float x,
  float y,
  float r,
  uint16_t color)
{
  canvas.drawCircle(
    (int)x,
    (int)y,
    (int)r,
    color
  );
}


void fillCircle(
  float x,
  float y,
  float r,
  uint16_t color)
{
  canvas.fillCircle(
    (int)x,
    (int)y,
    (int)r,
    color
  );
}


void line(
  float x1,
  float y1,
  float x2,
  float y2,
  uint16_t color)
{
  canvas.drawLine(
    (int)x1,
    (int)y1,
    (int)x2,
    (int)y2,
    color
  );
}


void rect(
  int x,
  int y,
  int w,
  int h,
  uint16_t color)
{
  canvas.drawRect(
    x,
    y,
    w,
    h,
    color
  );
}


void roundRect(
  int x,
  int y,
  int w,
  int h,
  int r,
  uint16_t color)
{
  canvas.drawRoundRect(
    x,
    y,
    w,
    h,
    r,
    color
  );
}


void fillRoundRect(
  int x,
  int y,
  int w,
  int h,
  int r,
  uint16_t color)
{
  canvas.fillRoundRect(
    x,
    y,
    w,
    h,
    r,
    color
  );
}


// ================================================================
// TEXT
// ================================================================

void centerText(
  const char *text,
  int x,
  int y,
  uint16_t color)
{
  canvas.setTextDatum(
    middle_center
  );

  canvas.setTextColor(
    color
  );

  canvas.setTextSize(1);

  canvas.drawString(
    text,
    x,
    y
  );
}


// ================================================================
// BACKGROUND
// ================================================================

void drawBackground()
{
  canvas.fillScreen(BLACK);

  // Very subtle blue inner field.

  fillCircle(
    120,
    120,
    102,
    NAVY
  );

  fillCircle(
    120,
    120,
    98,
    BLACK
  );


  // Small fixed mechanical marks.

  for (int i = 0; i < 24; i++)
  {
    float angle = i * 15.0f;

    float r1 = 93;
    float r2 = 97;

    line(
      px(120, r1, angle),
      py(120, r1, angle),

      px(120, r2, angle),
      py(120, r2, angle),

      BLUE_DARK
    );
  }
}


// ================================================================
// OUTER RING
// ================================================================

void drawOuterRing()
{
  circle(
    120,
    120,
    114,
    BLUE_DARK
  );

  circle(
    120,
    120,
    111,
    BLUE
  );

  circle(
    120,
    120,
    108,
    BLACK
  );


  float t = millis() / 1000.0f;

  float rotation =
    fmodf(
      t * 7.0f,
      360.0f
    );


  // Slow moving blue highlight.

  for (int i = 0; i < 7; i++)
  {
    float angle =
      rotation +
      i * 3.0f;

    line(
      px(120, 109, angle),
      py(120, 109, angle),

      px(120, 104, angle),
      py(120, 104, angle),

      i == 0
        ? CYAN
        : BLUE
    );
  }


  // Mechanical ticks.

  for (int i = 0; i < 36; i++)
  {
    float angle =
      i * 10.0f;

    float r1 =
      (i % 3 == 0)
      ? 105
      : 107;

    float r2 =
      (i % 3 == 0)
      ? 100
      : 104;

    line(
      px(120, r1, angle),
      py(120, r1, angle),

      px(120, r2, angle),
      py(120, r2, angle),

      i % 3 == 0
        ? BLUE
        : BLUE_DARK
    );
  }
}


// ================================================================
// HEADER
// ================================================================

void drawHeader()
{
  centerText(
    "R2D2 COMPLEX",
    120,
    38,
    WHITE_BLUE
  );

  centerText(
    "MKIV",
    120,
    50,
    CYAN
  );

  line(
    76,
    58,
    164,
    58,
    BLUE
  );
}


// ================================================================
// FOOTER
// ================================================================

void drawFooter()
{
  line(
    65,
    180,
    175,
    180,
    BLUE
  );


  centerText(
    "ASTROMECH DIAGNOSTIC SYSTEM",
    120,
    190,
    BLUE_LIGHT
  );


  float t =
    millis() / 1000.0f;


  // Slow status pulse.

  float p =
    (sinf(t * 1.2f) + 1.0f) * 0.5f;


  fillCircle(
    90,
    202,
    2 + p,
    CYAN
  );

  fillCircle(
    120,
    202,
    2,
    BLUE_LIGHT
  );

  fillCircle(
    150,
    202,
    2,
    BLUE
  );
}


// ================================================================
// CENTRAL FRAME
// ================================================================

void drawFrame()
{
  roundRect(
    42,
    67,
    156,
    100,
    10,
    BLUE
  );

  roundRect(
    46,
    71,
    148,
    92,
    7,
    BLUE_DARK
  );


  // Corner details.

  line(52, 78, 52, 87, CYAN);
  line(52, 78, 61, 78, CYAN);

  line(188, 78, 188, 87, CYAN);
  line(188, 78, 179, 78, CYAN);

  line(52, 156, 52, 147, CYAN);
  line(52, 156, 61, 156, CYAN);

  line(188, 156, 188, 147, CYAN);
  line(188, 156, 179, 156, CYAN);
}


// ================================================================
// SCENE LABEL
// ================================================================

void drawSceneLabel()
{
  const char *label;

  switch (scene)
  {
    case SCENE_OPTICS:
      label = "OPTICAL ARRAY";
      break;

    case SCENE_GYRO:
      label = "GYRO STABILIZER";
      break;

    case SCENE_RADAR:
      label = "TARGET ANALYSIS";
      break;

    case SCENE_POWER:
      label = "POWER DISTRIBUTION";
      break;

    case SCENE_MEMORY:
      label = "MEMORY MATRIX";
      break;

    case SCENE_SERVO:
      label = "SERVO CALIBRATION";
      break;

    case SCENE_SIGNAL:
      label = "SIGNAL ANALYSIS";
      break;

    case SCENE_HOLOGRAM:
      label = "HOLOGRAPHIC LINK";
      break;

    case SCENE_NAVIGATION:
      label = "NAVIGATION SYSTEM";
      break;

    default:
      label = "ASTROMECH CORE";
      break;
  }


  centerText(
    label,
    120,
    145,
    WHITE_BLUE
  );

  centerText(
    "SYSTEM NOMINAL",
    120,
    156,
    BLUE_LIGHT
  );
}


// ================================================================
// SCENE 1
// ASTROMECH CORE
// ================================================================

void drawCore()
{
  float t = sceneTime();

  float cx = 120;
  float cy = 111;


  circle(
    cx,
    cy,
    38,
    BLUE
  );

  circle(
    cx,
    cy,
    35,
    BLUE_DARK
  );

  circle(
    cx,
    cy,
    31,
    BLACK
  );


  // Three independent mechanical rings.

  for (int ring = 0; ring < 3; ring++)
  {
    float radius =
      27 - ring * 7;

    float speed =
      10.0f +
      ring * 4.0f;

    float angle =
      fmodf(
        t * speed *
        (ring % 2 ? -1.0f : 1.0f),
        360.0f
      );


    for (int i = 0; i < 8; i++)
    {
      float a =
        angle + i * 45.0f;

      float a2 =
        a + 20.0f;

      line(
        px(cx, radius, a),
        py(cy, radius, a),

        px(cx, radius, a2),
        py(cy, radius, a2),

        i == 0
          ? CYAN
          : BLUE
      );
    }
  }


  // Central eye.

  float pulse =
    (sinf(t * 1.3f) + 1.0f) * 0.5f;


  fillCircle(
    cx,
    cy,
    9 + pulse * 2,
    BLUE
  );

  fillCircle(
    cx,
    cy,
    6 + pulse,
    CYAN
  );

  fillCircle(
    cx - 2,
    cy - 2,
    2,
    WHITE
  );


  // Four tiny status points.

  for (int i = 0; i < 4; i++)
  {
    float a =
      45 + i * 90;

    float p =
      (sinf(
        t * 1.1f +
        i
      ) + 1.0f) * 0.5f;


    fillCircle(
      px(cx, 26, a),
      py(cy, 26, a),
      1.5f + p,
      p > 0.5f
        ? CYAN
        : BLUE
    );
  }
}


// ================================================================
// SCENE 2
// OPTICAL SCANNER
// ================================================================

void drawOptics()
{
  float t = sceneTime();

  float cx = 120;
  float cy = 111;


  circle(cx, cy, 38, BLUE);
  circle(cx, cy, 34, BLUE_DARK);
  circle(cx, cy, 29, BLACK);


  // Radar rings.

  circle(cx, cy, 21, BLUE_DARK);
  circle(cx, cy, 11, BLUE_DARK);


  // Crosshair.

  line(84, cy, 156, cy, BLUE_DARK);
  line(cx, 75, cx, 147, BLUE_DARK);


  // Rotating scanner.

  float angle =
    fmodf(
      t * 32.0f,
      360.0f
    );


  line(
    cx,
    cy,

    px(cx, 34, angle),
    py(cy, 34, angle),

    CYAN
  );


  // Scanner glow.

  for (int i = 1; i <= 4; i++)
  {
    float a =
      angle - i * 5;

    line(
      cx,
      cy,

      px(cx, 33, a),
      py(cy, 33, a),

      i == 4
        ? BLUE_DARK
        : BLUE
    );
  }


  // Moving optical target.

  float targetAngle =
    fmodf(
      60 +
      t * 9,
      360
    );

  float targetRadius =
    18 +
    sinf(t * 0.7f) * 8;


  float tx =
    px(cx, targetRadius, targetAngle);

  float ty =
    py(cy, targetRadius, targetAngle);


  circle(tx, ty, 7, BLUE);
  fillCircle(tx, ty, 3, CYAN);
}


// ================================================================
// SCENE 3
// GYROSCOPE
// ================================================================

void drawGyro()
{
  float t = sceneTime();

  float cx = 120;
  float cy = 111;


  circle(cx, cy, 38, BLUE);
  circle(cx, cy, 34, BLUE_DARK);
  circle(cx, cy, 29, BLACK);


  // Three slow orbital rings.

  float angles[3];

  angles[0] = t * 14;
  angles[1] = -t * 20;
  angles[2] = t * 27;


  for (int r = 0; r < 3; r++)
  {
    float radius =
      29 - r * 8;

    float a =
      angles[r];


    float x1 =
      px(cx, radius, a);

    float y1 =
      py(cy, radius, a);


    float x2 =
      px(cx, radius, a + 180);

    float y2 =
      py(cy, radius, a + 180);


    line(
      x1,
      y1,
      x2,
      y2,
      r == 0
        ? CYAN
        : BLUE
    );


    fillCircle(
      x1,
      y1,
      3,
      r == 0
        ? CYAN
        : BLUE_LIGHT
    );
  }


  fillCircle(
    cx,
    cy,
    4,
    WHITE_BLUE
  );
}


// ================================================================
// SCENE 4
// RADAR / TARGET ANALYSIS
// ================================================================

void drawRadar()
{
  float t = sceneTime();

  float cx = 120;
  float cy = 111;


  circle(cx, cy, 38, BLUE);
  circle(cx, cy, 32, BLUE_DARK);
  circle(cx, cy, 24, BLUE_DARK);
  circle(cx, cy, 15, BLUE_DARK);


  float angle =
    fmodf(
      t * 22,
      360
    );


  line(
    cx,
    cy,

    px(cx, 36, angle),
    py(cy, 36, angle),

    CYAN
  );


  // Three slowly moving targets.

  for (int i = 0; i < 3; i++)
  {
    float a =
      40 +
      i * 115 +
      sinf(t * 0.25f + i) * 12;

    float r =
      12 +
      i * 6;

    float x =
      px(cx, r, a);

    float y =
      py(cy, r, a);


    circle(
      x,
      y,
      4,
      BLUE
    );

    fillCircle(
      x,
      y,
      2,
      i == 0
        ? CYAN
        : BLUE_LIGHT
    );
  }
}


// ================================================================
// SCENE 5
// POWER DISTRIBUTION
// ================================================================

void drawPower()
{
  float t = sceneTime();


  // Main housing.

  roundRect(
    61,
    82,
    118,
    43,
    6,
    BLUE
  );


  roundRect(
    65,
    86,
    110,
    35,
    4,
    BLUE_DARK
  );


  // Eight energy cells.

  for (int i = 0; i < 8; i++)
  {
    float wave =
      sinf(
        t * 0.8f -
        i * 0.45f
      );


    float level =
      (wave + 1) * 0.5f;


    int h =
      8 +
      level * 19;


    fillRoundRect(
      70 + i * 12,
      114 - h,
      7,
      h,
      2,
      level > 0.55
        ? BLUE_LIGHT
        : BLUE
    );
  }


  // Energy transfer pulse.

  float x =
    70 +
    fmodf(
      t * 15,
      88
    );


  fillCircle(
    x,
    132,
    3,
    CYAN
  );


  line(
    69,
    132,
    171,
    132,
    BLUE_DARK
  );
}


// ================================================================
// SCENE 6
// MEMORY MATRIX
// ================================================================

void drawMemory()
{
  float t = sceneTime();


  // Memory blocks.

  for (int row = 0; row < 5; row++)
  {
    for (int col = 0; col < 9; col++)
    {
      float phase =
        t * 0.7f +
        row * 0.7f +
        col * 0.25f;


      float level =
        (sinf(phase) + 1) * 0.5f;


      uint16_t color =
        level > 0.65
        ? BLUE_LIGHT
        : BLUE_DARK;


      fillRoundRect(
        62 + col * 13,
        78 + row * 10,
        9,
        6,
        2,
        color
      );
    }
  }


  // Memory scanner.

  float scan =
    62 +
    fmodf(
      t * 9,
      116
    );


  line(
    scan,
    75,
    scan,
    132,
    CYAN
  );
}


// ================================================================
// SCENE 7
// SERVO CALIBRATION
// ================================================================

void drawServo()
{
  float t = sceneTime();


  // Four actuator tracks.

  for (int i = 0; i < 4; i++)
  {
    int y =
      80 + i * 13;


    roundRect(
      61,
      y,
      118,
      8,
      3,
      BLUE
    );


    float position =
      (sinf(
        t * 0.65f +
        i * 0.8f
      ) + 1) * 0.5f;


    int x =
      64 +
      position * 105;


    fillRoundRect(
      x,
      y + 2,
      9,
      4,
      2,
      CYAN
    );
  }


  // Calibration axis.

  line(
    68,
    134,
    172,
    134,
    BLUE_DARK
  );


  float marker =
    68 +
    (sinf(t * 0.45f) + 1) *
    52;


  fillCircle(
    marker,
    134,
    3,
    BLUE_LIGHT
  );
}


// ================================================================
// SCENE 8
// SIGNAL ANALYSIS
// ================================================================

void drawSignal()
{
  float t = sceneTime();


  // Background grid.

  for (int y = 82; y <= 132; y += 10)
  {
    line(
      59,
      y,
      181,
      y,
      BLUE_DARK
    );
  }


  for (int x = 65; x <= 175; x += 15)
  {
    line(
      x,
      78,
      x,
      136,
      BLUE_DARK
    );
  }


  // Waveform.

  float oldX = 61;
  float oldY = 108;


  for (int x = 63; x <= 179; x += 2)
  {
    float y =
      108 +

      sinf(
        (x * 0.14f) +
        t * 1.8f
      ) * 9 +

      sinf(
        (x * 0.31f) -
        t * 0.9f
      ) * 3;


    line(
      oldX,
      oldY,
      x,
      y,
      CYAN
    );


    oldX = x;
    oldY = y;
  }


  // Moving signal marker.

  float marker =
    62 +
    fmodf(
      t * 13,
      116
    );


  fillCircle(
    marker,
    108,
    3,
    WHITE_BLUE
  );
}


// ================================================================
// SCENE 9
// HOLOGRAPHIC SWEEP
// ================================================================

void drawHologram()
{
  float t = sceneTime();

  float cx = 120;
  float cy = 110;


  // Elliptical-style construction.

  circle(
    cx,
    cy,
    35,
    BLUE
  );

  circle(
    cx,
    cy,
    28,
    BLUE_DARK
  );


  // Horizontal holographic bands.

  for (int i = -3; i <= 3; i++)
  {
    float y =
      cy +
      i * 7;


    line(
      86,
      y,
      154,
      y,
      i == 0
        ? BLUE_LIGHT
        : BLUE_DARK
    );
  }


  // Vertical scanning plane.

  float scan =
    88 +
    fmodf(
      t * 14,
      64
    );


  for (int i = -2; i <= 2; i++)
  {
    line(
      scan + i * 2,
      79,
      scan + i * 2,
      141,
      i == 0
        ? CYAN
        : BLUE
    );
  }


  // Holographic center.

  fillCircle(
    cx,
    cy,
    5,
    CYAN
  );


  fillCircle(
    cx,
    cy,
    2,
    WHITE
  );
}


// ================================================================
// SCENE 10
// NAVIGATION
// ================================================================

void drawNavigation()
{
  float t = sceneTime();

  float cx = 120;
  float cy = 111;


  circle(
    cx,
    cy,
    38,
    BLUE
  );

  circle(
    cx,
    cy,
    34,
    BLUE_DARK
  );


  // Compass ticks.

  for (int i = 0; i < 16; i++)
  {
    float a =
      i * 22.5f;


    float r1 =
      (i % 4 == 0)
      ? 33
      : 35;


    float r2 = 29;


    line(
      px(cx, r1, a),
      py(cy, r1, a),

      px(cx, r2, a),
      py(cy, r2, a),

      i % 4 == 0
        ? CYAN
        : BLUE
    );
  }


  // Rotating heading.

  float heading =
    fmodf(
      t * 8,
      360
    );


  line(
    cx,
    cy,

    px(cx, 27, heading),
    py(cy, 27, heading),

    WHITE_BLUE
  );


  fillCircle(
    px(cx, 27, heading),
    py(cy, 27, heading),
    3,
    CYAN
  );


  // Center.

  fillCircle(
    cx,
    cy,
    4,
    BLUE_LIGHT
  );
}


// ================================================================
// DRAW CURRENT SCENE
// ================================================================

void drawScene()
{
  drawFrame();


  switch (scene)
  {
    case SCENE_CORE:
      drawCore();
      break;

    case SCENE_OPTICS:
      drawOptics();
      break;

    case SCENE_GYRO:
      drawGyro();
      break;

    case SCENE_RADAR:
      drawRadar();
      break;

    case SCENE_POWER:
      drawPower();
      break;

    case SCENE_MEMORY:
      drawMemory();
      break;

    case SCENE_SERVO:
      drawServo();
      break;

    case SCENE_SIGNAL:
      drawSignal();
      break;

    case SCENE_HOLOGRAM:
      drawHologram();
      break;

    case SCENE_NAVIGATION:
      drawNavigation();
      break;
  }


  drawSceneLabel();
}


// ================================================================
// BOOT SCREEN
// ================================================================

void drawBoot()
{
  canvas.fillScreen(BLACK);


  circle(
    120,
    120,
    105,
    BLUE_DARK
  );

  circle(
    120,
    120,
    101,
    BLUE
  );

  circle(
    120,
    120,
    97,
    BLACK
  );


  centerText(
    "R2D2",
    120,
    91,
    WHITE_BLUE
  );


  centerText(
    "COMPLEX MKIV",
    120,
    110,
    CYAN
  );


  roundRect(
    67,
    127,
    106,
    8,
    4,
    BLUE
  );


  fillRoundRect(
    70,
    130,
    100,
    2,
    1,
    CYAN
  );


  centerText(
    "SYSTEM INITIALIZING",
    120,
    150,
    BLUE_LIGHT
  );


  canvas.pushSprite(
    0,
    0
  );
}


// ================================================================
// RENDER
// ================================================================

void render()
{
  drawBackground();

  drawHeader();

  drawScene();

  drawFooter();

  drawOuterRing();


  canvas.pushSprite(
    0,
    0
  );
}


// ================================================================
// SETUP
// ================================================================

void setup()
{
  Serial.begin(115200);


  // Backlight.

  pinMode(
    3,
    OUTPUT
  );

  digitalWrite(
    3,
    HIGH
  );


  // Initialize display.

  lcd.init();

  lcd.setRotation(0);

  lcd.setColorDepth(16);


  // Create full-screen sprite.

  canvas.setColorDepth(16);

  canvas.createSprite(
    W,
    H
  );


  // Boot animation.

  drawBoot();

  delay(1500);


  // Start main animation.

  setScene(
    SCENE_CORE,
    12000
  );


  lastFrame =
    millis();
}


// ================================================================
// MAIN LOOP
// ================================================================

void loop()
{
  uint32_t now =
    millis();


  // Stable frame timing.

  if (
    now - lastFrame <
    FRAME_TIME
  )
  {
    return;
  }


  lastFrame =
    now;


  // Change scene.

  if (
    now - sceneStart >=
    sceneLength
  )
  {
    nextScene();
  }


  render();
}
