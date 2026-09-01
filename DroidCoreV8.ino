/*
  ================================================================
  R2D2 COMPLEX MKIV
  Autonomous Astromech Diagnostic Display

  Hardware:
  - ESP32-C3
  - 1.28" 240x240 GC9A01 round display
  - LovyanGFX

  Features:
  - No WiFi
  - No buttons required
  - No sound
  - Fully autonomous
  - Fast animated diagnostics
  - Multiple independent system screens
  - Blue / cyan / white color palette
  - Full-screen scene changes
  - Off-screen rendering for stable animation

  Scenes:
  01 POWER CORE
  02 BATTERY CELLS
  03 HYDRAULIC SYSTEM
  04 LIFE SUPPORT
  05 MOTOR SYSTEM
  06 SERVO ARRAY
  07 THERMAL SYSTEM
  08 NAVIGATION
  09 COMMUNICATIONS
  10 MEMORY MATRIX
  11 OPTICAL SYSTEM
  12 RADAR
  13 GYRO STABILIZER
  14 POWER DISTRIBUTION
  15 ASTROMECH CORE
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
    bus.freq_read = 20000000;

    bus.spi_3wire = false;
    bus.use_lock = true;
    bus.dma_channel = SPI_DMA_CH_AUTO;

    bus.pin_sclk = 6;
    bus.pin_mosi = 7;
    bus.pin_miso = -1;
    bus.pin_dc = 2;

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
LGFX_Sprite screen(&lcd);


// ================================================================
// DISPLAY CONSTANTS
// ================================================================

constexpr int W = 240;
constexpr int H = 240;

constexpr uint32_t FRAME_TIME = 16;


// ================================================================
// COLORS
// ================================================================

constexpr uint16_t BLACK       = 0x0000;
constexpr uint16_t BLUE_BLACK  = 0x0008;
constexpr uint16_t BLUE_DARK   = 0x0018;
constexpr uint16_t BLUE_MID    = 0x025F;
constexpr uint16_t BLUE        = 0x03FF;
constexpr uint16_t BLUE_LIGHT  = 0x05FF;
constexpr uint16_t CYAN        = 0x07FF;
constexpr uint16_t WHITE_BLUE  = 0xDFFF;
constexpr uint16_t WHITE       = 0xFFFF;


// ================================================================
// SCENES
// ================================================================

enum Scene
{
  POWER_CORE,
  BATTERY,
  HYDRAULIC,
  LIFE_SUPPORT,
  MOTORS,
  SERVOS,
  THERMAL,
  NAVIGATION,
  COMMS,
  MEMORY,
  OPTICS,
  RADAR,
  GYRO,
  DISTRIBUTION,
  ASTROMECH_CORE
};


Scene currentScene = POWER_CORE;

uint32_t sceneStarted = 0;
uint32_t sceneDuration = 5200;

uint32_t lastFrame = 0;


// ================================================================
// RANDOM / VALUES
// ================================================================

float noiseValue(
  int seed,
  float speed)
{
  return
    (sinf(
      millis() * 0.001f * speed +
      seed * 1.731f
    ) + 1.0f) * 0.5f;
}


// ================================================================
// GEOMETRY
// ================================================================

float rad(float a)
{
  return a * PI / 180.0f;
}


float circleX(
  float cx,
  float radius,
  float angle)
{
  return cx + cosf(rad(angle)) * radius;
}


float circleY(
  float cy,
  float radius,
  float angle)
{
  return cy + sinf(rad(angle)) * radius;
}


// ================================================================
// TEXT HELPERS
// ================================================================

void text(
  const char *str,
  int x,
  int y,
  uint16_t color = WHITE_BLUE)
{
  screen.setTextDatum(middle_center);
  screen.setTextSize(1);
  screen.setTextColor(color);
  screen.drawString(str, x, y);
}


void textLeft(
  const char *str,
  int x,
  int y,
  uint16_t color = WHITE_BLUE)
{
  screen.setTextDatum(middle_left);
  screen.setTextSize(1);
  screen.setTextColor(color);
  screen.drawString(str, x, y);
}


// ================================================================
// BASIC GRAPHICS
// ================================================================

void hLine(
  int x1,
  int x2,
  int y,
  uint16_t color)
{
  screen.drawLine(
    x1,
    y,
    x2,
    y,
    color
  );
}


void vLine(
  int x,
  int y1,
  int y2,
  uint16_t color)
{
  screen.drawLine(
    x,
    y1,
    x,
    y2,
    color
  );
}


void panel(
  int x,
  int y,
  int w,
  int h,
  uint16_t color = BLUE_DARK)
{
  screen.drawRoundRect(
    x,
    y,
    w,
    h,
    6,
    color
  );
}


void filledPanel(
  int x,
  int y,
  int w,
  int h,
  uint16_t color)
{
  screen.fillRoundRect(
    x,
    y,
    w,
    h,
    6,
    color
  );
}


// ================================================================
// STANDARD HEADER
// ================================================================

void header(
  const char *title,
  const char *subtitle)
{
  text(
    title,
    120,
    35,
    WHITE
  );

  text(
    subtitle,
    120,
    49,
    CYAN
  );

  hLine(
    55,
    185,
    59,
    BLUE_DARK
  );
}


// ================================================================
// STANDARD FOOTER
// ================================================================

void footer(
  const char *status)
{
  hLine(
    55,
    185,
    184,
    BLUE_DARK
  );

  text(
    status,
    120,
    198,
    BLUE_LIGHT
  );

  float p =
    noiseValue(88, 2.4f);

  screen.fillCircle(
    75,
    198,
    2 + p,
    CYAN
  );
}


// ================================================================
// OUTER DECORATION
// ================================================================

void outerDecoration()
{
  screen.drawCircle(
    120,
    120,
    113,
    BLUE_BLACK
  );

  screen.drawCircle(
    120,
    120,
    109,
    BLUE_DARK
  );


  float angle =
    fmodf(
      millis() * 0.035f,
      360.0f
    );


  for (int i = 0; i < 12; i++)
  {
    float a =
      angle + i * 30;


    int r1 =
      (i % 3 == 0)
      ? 108
      : 106;


    int r2 =
      (i % 3 == 0)
      ? 101
      : 103;


    screen.drawLine(
      circleX(120, r1, a),
      circleY(120, r1, a),
      circleX(120, r2, a),
      circleY(120, r2, a),
      i == 0 ? CYAN : BLUE
    );
  }
}


// ================================================================
// SCENE 01
// POWER CORE
// ================================================================

void drawPowerCore()
{
  header(
    "POWER CORE",
    "REACTOR MANAGEMENT"
  );


  float t =
    millis() * 0.001f;


  // Central reactor.

  screen.drawCircle(
    120,
    113,
    43,
    BLUE_DARK
  );

  screen.drawCircle(
    120,
    113,
    37,
    BLUE
  );

  screen.drawCircle(
    120,
    113,
    29,
    BLACK
  );


  // Rotating reactor segments.

  for (int i = 0; i < 12; i++)
  {
    float a =
      t * 80 +
      i * 30;


    float intensity =
      (sinf(
        t * 5 +
        i
      ) + 1) * 0.5f;


    uint16_t c =
      intensity > 0.55
      ? CYAN
      : BLUE;


    screen.drawLine(
      circleX(120, 34, a),
      circleY(113, 34, a),
      circleX(120, 39, a),
      circleY(113, 39, a),
      c
    );
  }


  // Reactor core.

  float pulse =
    (sinf(t * 7) + 1) * 0.5f;


  screen.fillCircle(
    120,
    113,
    12 + pulse * 5,
    BLUE
  );


  screen.fillCircle(
    120,
    113,
    6 + pulse * 2,
    CYAN
  );


  screen.fillCircle(
    118,
    111,
    2,
    WHITE
  );


  // Power flow.

  for (int i = 0; i < 5; i++)
  {
    float x =
      72 +
      fmodf(
        t * (18 + i * 4) +
        i * 17,
        96
      );


    screen.fillCircle(
      x,
      155,
      2,
      CYAN
    );
  }


  text(
    "REACTOR OUTPUT",
    120,
    156,
    BLUE_LIGHT
  );


  footer(
    "OUTPUT STABLE"
  );
}


// ================================================================
// SCENE 02
// BATTERY
// ================================================================

void drawBattery()
{
  header(
    "BATTERY ARRAY",
    "CELL CONDITION MONITOR"
  );


  float t =
    millis() * 0.001f;


  // Large battery percentage.

  float level =
    81.0f +
    sinf(t * 0.7f) * 2.0f;


  char value[16];

  snprintf(
    value,
    sizeof(value),
    "%d%%",
    (int)level
  );


  text(
    value,
    120,
    87,
    WHITE
  );


  // Battery bar.

  panel(
    57,
    102,
    126,
    18,
    BLUE
  );


  screen.fillRoundRect(
    61,
    106,
    118 * (level / 100.0f),
    10,
    3,
    CYAN
  );


  // Cell voltages.

  for (int i = 0; i < 6; i++)
  {
    int x =
      56 + i * 22;


    float v =
      4.05f +
      noiseValue(
        i,
        0.5f
      ) * 0.12f;


    char s[12];

    snprintf(
      s,
      sizeof(s),
      "%.2f",
      v
    );


    text(
      s,
      x + 10,
      137,
      BLUE_LIGHT
    );


    screen.fillRect(
      x,
      147,
      18,
      4,
      BLUE_DARK
    );


    screen.fillRect(
      x,
      147,
      18 * (v - 4.0f) / 0.2f,
      4,
      CYAN
    );
  }


  text(
    "CELL VOLTAGE",
    120,
    161,
    BLUE_LIGHT
  );


  footer(
    "CHARGE CONDITION: NOMINAL"
  );
}


// ================================================================
// SCENE 03
// HYDRAULIC
// ================================================================

void drawHydraulic()
{
  header(
    "HYDRAULIC SYSTEM",
    "ACTUATOR RESERVOIR"
  );


  float t =
    millis() * 0.001f;


  float oil =
    72 +
    sinf(t * 0.8f) * 3;


  // Oil tank.

  screen.drawRoundRect(
    67,
    72,
    106,
    63,
    12,
    BLUE
  );


  screen.drawRoundRect(
    72,
    77,
    96,
    53,
    9,
    BLUE_DARK
  );


  float oilHeight =
    45 * oil / 100.0f;


  screen.fillRoundRect(
    77,
    125 - oilHeight,
    86,
    oilHeight,
    6,
    BLUE
  );


  // Oil bubbles.

  for (int i = 0; i < 6; i++)
  {
    float x =
      82 +
      fmodf(
        i * 17 +
        t * (5 + i),
        75
      );


    float y =
      121 -
      fmodf(
        t * (8 + i * 2) +
        i * 13,
        oilHeight
      );


    screen.fillCircle(
      x,
      y,
      1.5,
      CYAN
    );
  }


  char oilText[20];

  snprintf(
    oilText,
    sizeof(oilText),
    "OIL LEVEL %d%%",
    (int)oil
  );


  text(
    oilText,
    120,
    145,
    WHITE
  );


  float pressure =
    82 +
    sinf(t * 1.5f) * 4;


  char p[20];

  snprintf(
    p,
    sizeof(p),
    "PRESSURE %.1f PSI",
    pressure
  );


  text(
    p,
    120,
    159,
    BLUE_LIGHT
  );


  footer(
    "HYDRAULICS NOMINAL"
  );
}


// ================================================================
// SCENE 04
// LIFE SUPPORT
// ================================================================

void drawLifeSupport()
{
  header(
    "LIFE SUPPORT",
    "INTERNAL ATMOSPHERE"
  );


  float t =
    millis() * 0.001f;


  // Air reservoir.

  float air =
    91 +
    sinf(t * 0.45f) * 1.5f;


  text(
    "AIR RESERVE",
    120,
    76,
    BLUE_LIGHT
  );


  char airText[16];

  snprintf(
    airText,
    sizeof(airText),
    "%d%%",
    (int)air
  );


  text(
    airText,
    120,
    95,
    WHITE
  );


  // Air gauge.

  screen.drawCircle(
    120,
    122,
    28,
    BLUE
  );


  screen.drawCircle(
    120,
    122,
    22,
    BLUE_DARK
  );


  float gaugeAngle =
    -140 +
    air * 2.8f;


  screen.drawLine(
    120,
    122,
    circleX(120, 20, gaugeAngle),
    circleY(122, 20, gaugeAngle),
    CYAN
  );


  // O2 / pressure / filter.

  textLeft(
    "O2",
    63,
    160,
    BLUE_LIGHT
  );

  text(
    "94%",
    105,
    160,
    WHITE_BLUE
  );


  textLeft(
    "PRESS",
    127,
    160,
    BLUE_LIGHT
  );

  text(
    "101 kPa",
    170,
    160,
    WHITE_BLUE
  );


  footer(
    "ATMOSPHERE NOMINAL"
  );
}


// ================================================================
// SCENE 05
// MOTORS
// ================================================================

void drawMotors()
{
  header(
    "MOTOR SYSTEM",
    "DRIVE ASSEMBLY"
  );


  float t =
    millis() * 0.001f;


  for (int i = 0; i < 3; i++)
  {
    int y =
      78 + i * 27;


    textLeft(
      i == 0 ? "MOTOR A" :
      i == 1 ? "MOTOR B" :
               "AUX MOTOR",
      55,
      y,
      BLUE_LIGHT
    );


    // RPM value.

    float rpm =
      2100 +
      sinf(
        t * (1.2f + i * 0.2f) +
        i
      ) * 450;


    char rpmText[16];

    snprintf(
      rpmText,
      sizeof(rpmText),
      "%d RPM",
      (int)rpm
    );


    text(
      rpmText,
      163,
      y,
      WHITE_BLUE
    );


    // Load bar.

    screen.drawRoundRect(
      55,
      y + 8,
      130,
      7,
      3,
      BLUE_DARK
    );


    float load =
      0.45f +
      noiseValue(i + 20, 1.1f) * 0.3f;


    screen.fillRoundRect(
      58,
      y + 10,
      124 * load,
      3,
      2,
      CYAN
    );
  }


  footer(
    "MOTOR PERFORMANCE NOMINAL"
  );
}


// ================================================================
// SCENE 06
// SERVOS
// ================================================================

void drawServos()
{
  header(
    "SERVO ARRAY",
    "ACTUATOR CALIBRATION"
  );


  float t =
    millis() * 0.001f;


  for (int i = 0; i < 5; i++)
  {
    int y =
      75 + i * 18;


    // Track.

    screen.drawRoundRect(
      65,
      y,
      110,
      8,
      4,
      BLUE_DARK
    );


    // Moving actuator.

    float position =
      (sinf(
        t * (1.4f + i * 0.18f) +
        i
      ) + 1) * 0.5f;


    int x =
      69 +
      position * 98;


    screen.fillRoundRect(
      x,
      y + 2,
      12,
      4,
      2,
      CYAN
    );


    // Position marker.

    screen.fillCircle(
      65,
      y + 4,
      2,
      BLUE
    );


    screen.fillCircle(
      175,
      y + 4,
      2,
      BLUE
    );
  }


  text(
    "CALIBRATION RUN",
    120,
    174,
    BLUE_LIGHT
  );


  footer(
    "ALL ACTUATORS RESPONDING"
  );
}


// ================================================================
// SCENE 07
// THERMAL
// ================================================================

void drawThermal()
{
  header(
    "THERMAL SYSTEM",
    "INTERNAL TEMPERATURE"
  );


  float t =
    millis() * 0.001f;


  // Thermal zones.

  for (int i = 0; i < 6; i++)
  {
    int x =
      56 + i * 22;


    float temp =
      31 +
      noiseValue(
        i + 30,
        0.45f
      ) * 14;


    // Zone height.

    int h =
      25 +
      (temp - 30) * 2;


    screen.fillRoundRect(
      x,
      137 - h,
      15,
      h,
      3,
      temp > 40
        ? BLUE_LIGHT
        : BLUE
    );


    char s[8];

    snprintf(
      s,
      sizeof(s),
      "%d",
      (int)temp
    );


    text(
      s,
      x + 7,
      146,
      BLUE_LIGHT
    );
  }


  text(
    "°C",
    184,
    146,
    BLUE_LIGHT
  );


  float coreTemp =
    38 +
    sinf(t * 1.1f) * 2;


  char core[20];

  snprintf(
    core,
    sizeof(core),
    "CORE %.1f C",
    coreTemp
  );


  text(
    core,
    120,
    162,
    WHITE
  );


  footer(
    "THERMAL LOAD ACCEPTABLE"
  );
}


// ================================================================
// SCENE 08
// NAVIGATION
// ================================================================

void drawNavigation()
{
  header(
    "NAVIGATION",
    "ASTROMECH POSITIONING"
  );


  float t =
    millis() * 0.001f;


  // Compass.

  screen.drawCircle(
    120,
    111,
    42,
    BLUE
  );


  screen.drawCircle(
    120,
    111,
    35,
    BLUE_DARK
  );


  for (int i = 0; i < 16; i++)
  {
    float a =
      i * 22.5f;


    int r =
      i % 4 == 0
      ? 39
      : 37;


    screen.drawLine(
      circleX(120, r, a),
      circleY(111, r, a),
      circleX(120, 33, a),
      circleY(111, 33, a),
      i % 4 == 0
        ? CYAN
        : BLUE
    );
  }


  float heading =
    fmodf(
      t * 22,
      360
    );


  screen.drawLine(
    120,
    111,
    circleX(120, 30, heading),
    circleY(111, 30, heading),
    WHITE_BLUE
  );


  screen.fillCircle(
    circleX(120, 30, heading),
    circleY(111, 30, heading),
    3,
    CYAN
  );


  char hdg[20];

  snprintf(
    hdg,
    sizeof(hdg),
    "HDG %03d",
    (int)heading
  );


  text(
    hdg,
    120,
    164,
    WHITE
  );


  footer(
    "NAVIGATION LOCKED"
  );
}


// ================================================================
// SCENE 09
// COMMUNICATIONS
// ================================================================

void drawComms()
{
  header(
    "COMMUNICATIONS",
    "LONG RANGE TRANSCEIVER"
  );


  float t =
    millis() * 0.001f;


  // Signal arcs.

  for (int i = 0; i < 4; i++)
  {
    int radius =
      13 + i * 11;


    screen.drawArc(
      120,
      125,
      radius,
      radius,
      220,
      320,
      BLUE_LIGHT
    );
  }


  // Antenna center.

  screen.fillCircle(
    120,
    125,
    6,
    CYAN
  );


  // Signal pulses.

  for (int i = 0; i < 5; i++)
  {
    float p =
      fmodf(
        t * (1.5f + i * 0.25f) +
        i * 0.6f,
        1.0f
      );


    int x =
      66 +
      p * 108;


    screen.fillCircle(
      x,
      78 + i * 7,
      2,
      i == 0
        ? CYAN
        : BLUE
    );
  }


  text(
    "SIGNAL 87%",
    120,
    159,
    WHITE
  );


  footer(
    "TRANSMISSION READY"
  );
}


// ================================================================
// SCENE 10
// MEMORY
// ================================================================

void drawMemory()
{
  header(
    "MEMORY MATRIX",
    "DATA INTEGRITY CHECK"
  );


  float t =
    millis() * 0.001f;


  // Memory matrix.

  for (int row = 0; row < 6; row++)
  {
    for (int col = 0; col < 8; col++)
    {
      float value =
        noiseValue(
          row * 10 + col,
          1.4f
        );


      uint16_t c =
        value > 0.62f
        ? CYAN
        : BLUE_DARK;


      screen.fillRoundRect(
        59 + col * 16,
        74 + row * 12,
        11,
        7,
        2,
        c
      );
    }
  }


  // Fast scan line.

  float scan =
    57 +
    fmodf(
      t * 35,
      128
    );


  screen.drawLine(
    scan,
    70,
    scan,
    147,
    WHITE_BLUE
  );


  text(
    "MEMORY 68.4 GB",
    120,
    159,
    WHITE
  );


  footer(
    "CRC CHECK: PASS"
  );
}


// ================================================================
// SCENE 11
// OPTICS
// ================================================================

void drawOptics()
{
  header(
    "OPTICAL ARRAY",
    "PRIMARY VISUAL SENSOR"
  );


  float t =
    millis() * 0.001f;


  screen.drawCircle(
    120,
    111,
    43,
    BLUE_DARK
  );


  screen.drawCircle(
    120,
    111,
    36,
    BLUE
  );


  screen.drawCircle(
    120,
    111,
    29,
    BLACK
  );


  // Crosshair.

  hLine(
    84,
    156,
    111,
    BLUE_DARK
  );


  vLine(
    120,
    75,
    147,
    BLUE_DARK
  );


  // Rotating scan.

  float scan =
    t * 100;


  for (int i = 0; i < 8; i++)
  {
    float a =
      scan - i * 5;


    screen.drawLine(
      120,
      111,
      circleX(120, 33, a),
      circleY(111, 33, a),
      i == 0
        ? CYAN
        : BLUE
    );
  }


  // Moving target.

  float targetAngle =
    30 +
    sinf(t * 1.2f) * 120;


  float targetRadius =
    17 +
    sinf(t * 1.7f) * 8;


  float tx =
    circleX(120, targetRadius, targetAngle);


  float ty =
    circleY(111, targetRadius, targetAngle);


  screen.drawCircle(
    tx,
    ty,
    7,
    BLUE_LIGHT
  );


  screen.fillCircle(
    tx,
    ty,
    2,
    CYAN
  );


  footer(
    "OPTICAL SENSOR ACTIVE"
  );
}


// ================================================================
// SCENE 12
// RADAR
// ================================================================

void drawRadar()
{
  header(
    "TACTICAL RADAR",
    "LOCAL SPACE ANALYSIS"
  );


  float t =
    millis() * 0.001f;


  const int cx = 120;
  const int cy = 113;


  // Radar rings.

  screen.drawCircle(
    cx,
    cy,
    42,
    BLUE
  );

  screen.drawCircle(
    cx,
    cy,
    30,
    BLUE_DARK
  );

  screen.drawCircle(
    cx,
    cy,
    18,
    BLUE_DARK
  );


  // Crosshair.

  hLine(
    78,
    162,
    cy,
    BLUE_DARK
  );


  vLine(
    cx,
    71,
    155,
    BLUE_DARK
  );


  // Sweep.

  float sweep =
    fmodf(
      t * 110,
      360
    );


  for (int i = 0; i < 12; i++)
  {
    float a =
      sweep - i * 3;


    screen.drawLine(
      cx,
      cy,
      circleX(cx, 40, a),
      circleY(cy, 40, a),
      i == 0
        ? CYAN
        : BLUE
    );
  }


  // Targets.

  for (int i = 0; i < 5; i++)
  {
    float a =
      i * 72 +
      sinf(t * (0.5f + i * 0.1f)) * 35;


    float r =
      11 +
      i * 6;


    float x =
      circleX(cx, r, a);


    float y =
      circleY(cy, r, a);


    screen.fillCircle(
      x,
      y,
      i == 0 ? 3 : 2,
      i == 0 ? CYAN : BLUE_LIGHT
    );
  }


  footer(
    "5 CONTACTS TRACKED"
  );
}


// ================================================================
// SCENE 13
// GYRO
// ================================================================

void drawGyro()
{
  header(
    "GYRO STABILIZER",
    "MOTION CONTROL"
  );


  float t =
    millis() * 0.001f;


  // Artificial horizon.

  float pitch =
    sinf(t * 1.1f) * 12;


  float roll =
    sinf(t * 0.7f) * 18;


  screen.drawCircle(
    120,
    111,
    44,
    BLUE
  );


  screen.drawCircle(
    120,
    111,
    39,
    BLUE_DARK
  );


  // Rotating horizon line.

  float a =
    roll;


  screen.drawLine(
    circleX(120, 34, a + 180),
    circleY(111, 34, a + 180) + pitch,
    circleX(120, 34, a),
    circleY(111, 34, a) + pitch,
    CYAN
  );


  // Pitch lines.

  for (int i = -2; i <= 2; i++)
  {
    int y =
      111 +
      pitch +
      i * 10;


    screen.drawLine(
      97,
      y,
      143,
      y,
      BLUE_LIGHT
    );
  }


  // Center marker.

  screen.drawLine(
    108,
    111,
    132,
    111,
    WHITE_BLUE
  );


  screen.drawLine(
    120,
    103,
    120,
    119,
    WHITE_BLUE
  );


  char g[24];

  snprintf(
    g,
    sizeof(g),
    "ROLL %+03d°",
    (int)roll
  );


  text(
    g,
    120,
    163,
    WHITE
  );


  footer(
    "GYRO STABLE"
  );
}


// ================================================================
// SCENE 14
// POWER DISTRIBUTION
// ================================================================

void drawDistribution()
{
  header(
    "POWER DISTRIBUTION",
    "SYSTEM LOAD BALANCER"
  );


  float t =
    millis() * 0.001f;


  // Central power bus.

  screen.fillRoundRect(
    76,
    106,
    88,
    12,
    5,
    BLUE_DARK
  );


  screen.fillRoundRect(
    80,
    110,
    80,
    4,
    2,
    CYAN
  );


  // System nodes.

  const int nodeY[] =
  {
    77,
    91,
    137,
    151
  };


  const int nodeX[] =
  {
    67,
    67,
    173,
    173
  };


  const char *names[] =
  {
    "MOTORS",
    "LIFE",
    "OPTICS",
    "NAV"
  };


  for (int i = 0; i < 4; i++)
  {
    int x =
      nodeX[i];


    int y =
      nodeY[i];


    screen.drawRoundRect(
      x - 18,
      y - 5,
      36,
      10,
      4,
      BLUE
    );


    text(
      names[i],
      x,
      y,
      BLUE_LIGHT
    );


    // Energy packet moving toward bus.

    float p =
      fmodf(
        t * (0.6f + i * 0.15f),
        1.0f
      );


    float sx =
      x < 120
      ? x + 18
      : x - 18;


    float ex =
      x < 120
      ? 80
      : 160;


    float sy =
      y;


    float ey =
      112;


    float px =
      sx + (ex - sx) * p;


    float py =
      sy + (ey - sy) * p;


    screen.fillCircle(
      px,
      py,
      2,
      CYAN
    );
  }


  text(
    "LOAD 63%",
    120,
    137,
    WHITE
  );


  footer(
    "POWER ROUTING NOMINAL"
  );
}


// ================================================================
// SCENE 15
// ASTROMECH CORE
// ================================================================

void drawAstromechCore()
{
  header(
    "ASTROMECH CORE",
    "R2D2 COMPLEX MKIV"
  );


  float t =
    millis() * 0.001f;


  // Mechanical core rings.

  screen.drawCircle(
    120,
    111,
    45,
    BLUE_DARK
  );


  screen.drawCircle(
    120,
    111,
    39,
    BLUE
  );


  screen.drawCircle(
    120,
    111,
    33,
    BLACK
  );


  // Outer rotating segments.

  for (int i = 0; i < 10; i++)
  {
    float a =
      t * 70 +
      i * 36;


    screen.drawLine(
      circleX(120, 35, a),
      circleY(111, 35, a),
      circleX(120, 42, a),
      circleY(111, 42, a),
      i == 0
        ? CYAN
        : BLUE
    );
  }


  // Inner rotating ring.

  for (int i = 0; i < 6; i++)
  {
    float a =
      -t * 100 +
      i * 60;


    screen.drawLine(
      circleX(120, 24, a),
      circleY(111, 24, a),
      circleX(120, 29, a),
      circleY(111, 29, a),
      CYAN
    );
  }


  // Optical eye.

  float eyeX =
    120 +
    sinf(t * 1.8f) * 5;


  float eyeY =
    111 +
    cosf(t * 1.4f) * 3;


  float pulse =
    (sinf(t * 6) + 1) * 0.5f;


  screen.fillCircle(
    eyeX,
    eyeY,
    9 + pulse * 3,
    BLUE
  );


  screen.fillCircle(
    eyeX,
    eyeY,
    5 + pulse,
    CYAN
  );


  screen.fillCircle(
    eyeX - 2,
    eyeY - 2,
    2,
    WHITE
  );


  // Status lights.

  for (int i = 0; i < 4; i++)
  {
    float p =
      noiseValue(
        100 + i,
        2.5f
      );


    screen.fillCircle(
      circleX(120, 27, i * 90 + 45),
      circleY(111, 27, i * 90 + 45),
      2,
      p > 0.5f
        ? CYAN
        : BLUE
    );
  }


  footer(
    "R2D2 SYSTEMS NOMINAL"
  );
}


// ================================================================
// SCENE DISPATCH
// ================================================================

void drawScene()
{
  switch (currentScene)
  {
    case POWER_CORE:
      drawPowerCore();
      break;

    case BATTERY:
      drawBattery();
      break;

    case HYDRAULIC:
      drawHydraulic();
      break;

    case LIFE_SUPPORT:
      drawLifeSupport();
      break;

    case MOTORS:
      drawMotors();
      break;

    case SERVOS:
      drawServos();
      break;

    case THERMAL:
      drawThermal();
      break;

    case NAVIGATION:
      drawNavigation();
      break;

    case COMMS:
      drawComms();
      break;

    case MEMORY:
      drawMemory();
      break;

    case OPTICS:
      drawOptics();
      break;

    case RADAR:
      drawRadar();
      break;

    case GYRO:
      drawGyro();
      break;

    case DISTRIBUTION:
      drawDistribution();
      break;

    case ASTROMECH_CORE:
      drawAstromechCore();
      break;
  }
}


// ================================================================
// NEXT SCENE
// ================================================================

void nextScene()
{
  int next =
    ((int)currentScene + 1) % 15;


  currentScene =
    (Scene)next;


  sceneStarted =
    millis();


  // Slightly different timing per scene.

  switch (currentScene)
  {
    case ASTROMECH_CORE:
      sceneDuration = 6500;
      break;

    case RADAR:
      sceneDuration = 4300;
      break;

    case OPTICS:
      sceneDuration = 4500;
      break;

    case GYRO:
      sceneDuration = 4700;
      break;

    default:
      sceneDuration = 5000;
      break;
  }
}


// ================================================================
// BOOT SCREEN
// ================================================================

void bootScreen()
{
  screen.fillScreen(BLACK);


  // Outer circular structure.

  screen.drawCircle(
    120,
    120,
    108,
    BLUE_DARK
  );


  screen.drawCircle(
    120,
    120,
    103,
    BLUE
  );


  screen.drawCircle(
    120,
    120,
    98,
    BLACK
  );


  text(
    "R2D2",
    120,
    88,
    WHITE
  );


  text(
    "COMPLEX MKIV",
    120,
    105,
    CYAN
  );


  // Animated boot bar.

  for (int i = 0; i < 90; i += 3)
  {
    screen.fillRect(
      75 + i,
      123,
      2,
      5,
      i < 45
        ? CYAN
        : BLUE_DARK
    );


    screen.pushSprite(
      0,
      0
    );


    delay(12);
  }


  text(
    "INITIALIZING SYSTEMS",
    120,
    145,
    BLUE_LIGHT
  );


  screen.pushSprite(
    0,
    0
  );


  delay(700);
}


// ================================================================
// SETUP
// ================================================================

void setup()
{
  Serial.begin(115200);


  // Enable display backlight.

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


  // Create full-screen off-screen buffer.

  screen.setColorDepth(16);

  screen.createSprite(
    W,
    H
  );


  // Startup sequence.

  bootScreen();


  currentScene =
    POWER_CORE;


  sceneStarted =
    millis();


  sceneDuration =
    5000;


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


  // Maintain approximately 60 FPS.

  if (
    now - lastFrame <
    FRAME_TIME
  )
  {
    return;
  }


  lastFrame =
    now;


  // Change diagnostic system.

  if (
    now - sceneStarted >=
    sceneDuration
  )
  {
    nextScene();
  }


  // Render entire frame off-screen.

  screen.fillScreen(
    BLACK
  );


  drawScene();


  // Outer decoration is deliberately
  // drawn first conceptually by keeping
  // it around the outside of the UI.
  // It never covers the central graphics.

  outerDecoration();


  // Push the completed frame at once.

  screen.pushSprite(
    0,
    0
  );
}
