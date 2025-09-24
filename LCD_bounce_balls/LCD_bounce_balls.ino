#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);  // Create a sprite buffer

// ===== Ball data =====
const int NUM_BALLS = 6;

// Initial positions (non-overlapping, valid for 240x135 screen)
float x[NUM_BALLS]  = {40, 100, 160,  60, 180, 120};
float y[NUM_BALLS]  = {40,  80,  30, 100,  50,  90};

// Velocities
float dx[NUM_BALLS] = {2.5, 2.0, 1.5, 1.2, 1.0, 0.8};
float dy[NUM_BALLS] = {2.0, 2.5, 1.2, 1.0, 0.8, 0.6};

// Radii
int r[NUM_BALLS]  = {5, 8, 12, 15, 18, 20};

// Colors
uint16_t c[NUM_BALLS] = {
  TFT_RED, TFT_GREEN, TFT_BLUE,
  TFT_YELLOW, TFT_PURPLE, TFT_ORANGE
};

// ===== Collision helpers =====
bool isColliding(float x1, float y1, int r1, float x2, float y2, int r2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  float distSq = dx * dx + dy * dy;
  float radiusSum = r1 + r2;
  return distSq <= radiusSum * radiusSum;
}

void bounce(float &dx1, float &dy1, float &dx2, float &dy2) {
  float tempDx = dx1;
  float tempDy = dy1;
  dx1 = dx2;
  dy1 = dy2;
  dx2 = tempDx;
  dy2 = tempDy;
}

// Push balls apart if overlapping
void separate(float &x1, float &y1, int r1,
              float &x2, float &y2, int r2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  float dist = sqrt(dx * dx + dy * dy);
  if (dist == 0) return; // avoid divide by zero
  float overlap = (r1 + r2) - dist;
  if (overlap > 0) {
    float nx = dx / dist;
    float ny = dy / dist;
    // push each ball half the overlap apart
    x1 -= nx * overlap / 2;
    y1 -= ny * overlap / 2;
    x2 += nx * overlap / 2;
    y2 += ny * overlap / 2;
  }
}

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);  // Backlight on

  // Create sprite same size as screen
  sprite.createSprite(tft.width(), tft.height());
}

void loop() {
  // === Update positions ===
  for (int i = 0; i < NUM_BALLS; i++) {
    x[i] += dx[i];
    y[i] += dy[i];

    // Clamp + bounce off walls
    if (x[i] - r[i] < 0) {
      x[i] = r[i];
      dx[i] = -dx[i];
    } else if (x[i] + r[i] > tft.width()) {
      x[i] = tft.width() - r[i];
      dx[i] = -dx[i];
    }
    if (y[i] - r[i] < 0) {
      y[i] = r[i];
      dy[i] = -dy[i];
    } else if (y[i] + r[i] > tft.height()) {
      y[i] = tft.height() - r[i];
      dy[i] = -dy[i];
    }
  }

  // === Check ball-to-ball collisions ===
  for (int i = 0; i < NUM_BALLS; i++) {
    for (int j = i + 1; j < NUM_BALLS; j++) {
      if (isColliding(x[i], y[i], r[i], x[j], y[j], r[j])) {
        // Swap velocities (simple elastic collision)
        bounce(dx[i], dy[i], dx[j], dy[j]);
        // Push apart to avoid sticking
        separate(x[i], y[i], r[i], x[j], y[j], r[j]);
      }
    }
  }

  // === Draw frame ===
  sprite.fillSprite(TFT_BLACK);
  for (int i = 0; i < NUM_BALLS; i++) {
    sprite.fillCircle((int)x[i], (int)y[i], r[i], c[i]);
  }
  sprite.pushSprite(0, 0);

  delay(10); // Adjust speed
}
