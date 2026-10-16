// Texte centré horizontalement sur x (police intégrée : 6 x 8 pixels par caractère)
void drawCentered(const char *s, int x, int y, int size, uint16_t fg, uint16_t bg) {
  gfx->setTextSize(size);
  gfx->setTextColor(fg, bg);
  gfx->setCursor(x - (int)strlen(s) * 6 * size / 2, y);
  gfx->print(s);
}

void drawButton(const Button &b, const char *label, uint16_t fill, int size = 2) {
  // Texte foncé sur le seul remplissage clair (COL_SHORT), clair partout ailleurs
  uint16_t fg = (fill == COL_SHORT) ? COL_BG : COL_TEXT;
  gfx->fillRoundRect(b.x, b.y, b.w, b.h, 8, fill);
  drawCentered(label, b.x + b.w / 2, b.y + (b.h - 8 * size) / 2, size, fg, fill);
}

void drawStartButton() {
  const char *label = state == RUNNING ? "PAUSE" : state == PAUSED ? "REPRISE" : "DEMARRER";
  drawButton(BTN_START, label, state == RUNNING ? COL_BTN : phaseColor(), state == READY ? 2 : 3);
}

void drawCycleDots() {
  int n = settings[3];
  for (int i = 0; i < n; i++) {
    int x = CX - (n - 1) * 11 + i * 22;
    gfx->fillCircle(x, CY + 85, 6, i < doneInCycle ? COL_WORK : COL_TRACK);
  }
}

void drawTotal() {
  char buf[16];
  snprintf(buf, sizeof(buf), "Faits : %d", doneTotal);
  drawCentered(buf, BTN_START.x + BTN_START.w / 2, 20, 2, COL_DIM, COL_BG);
}

// Position à l'écran du GIF en cours de décodage (passée via pUser)
struct Origin { int16_t x, y; };
const Origin FOND_ORIGIN = {0, 0};
const Origin ZZZ_ORIGIN = {ZZZ_X, ZZZ_Y};

// Emplacement du chrono et du nom de la phase sur l'écran d'animation
// (pause : en bas à droite, à côté du chat ; travail : au centre, sous le ciel)
int animInfoX() { return isBreak() ? 410 : 240; }
int animTimeY() { return isBreak() ? 220 : 152; }
int animLabelY() { return animTimeY() + 45; }

void gifDraw(GIFDRAW *d) {
  static uint16_t line[480];
  const Origin *o = (const Origin *)d->pUser;
  int y = o->y + d->iY + d->y;
  const uint8_t *src = d->pPixels;
  int x = 0;
  while (x < d->iWidth) {
    // Les pixels transparents ne sont pas dessinés : on envoie chaque segment opaque
    while (x < d->iWidth && d->ucHasTransparency && src[x] == d->ucTransparent) x++;
    int start = x, n = 0;
    while (x < d->iWidth && !(d->ucHasTransparency && src[x] == d->ucTransparent)) line[n++] = d->pPalette[src[x++]];
    if (n) gfx->draw16bitRGBBitmap(o->x + d->iX + start, y, line, n, 1);
  }
}

void playZzzFrame() {
  int delayMs;
  if (zzzGif.playFrame(false, &delayMs, (void *)&ZZZ_ORIGIN) <= 0) zzzGif.reset();  // dernière image : on reboucle
  nextGifMs = millis() + delayMs;
}

void drawCloud(float cx, int cy, float k) {
  static const int8_t puffs[][3] = {{-30, 8, 16}, {-10, -4, 22}, {14, -10, 20}, {34, 4, 16}};
  for (auto &p : puffs) sky->fillCircle(cx + p[0] * k, cy + p[1] * k, p[2] * k, COL_CLOUD);
  sky->fillRoundRect(cx - 46 * k, cy - 4 * k, 96 * k, 28 * k, 12 * k, COL_CLOUD);
}

// Redessine le ciel : soleil fixe et nuages dont la position dépend de l'heure,
// donc la vitesse de défilement ne dépend pas de la fluidité de l'affichage
void drawSky() {
  struct Cloud { float x0; int y; float k, speed; };  // speed en pixels par seconde
  static const Cloud clouds[] = {{90, 40, 1.0, 14}, {330, 72, 1.2, 9}, {200, 102, 0.8, 20}};
  float t = millis() / 1000.0f;
  sky->fillScreen(COL_BG);
  sky->fillCircle(410, 55, 28, COL_SUN);
  for (auto &c : clouds) {
    float span = 480 + 120 * c.k;  // le nuage sort à gauche et revient par la droite
    float x = fmodf(c.x0 - c.speed * t, span);
    if (x < 0) x += span;
    drawCloud(x - 60 * c.k, c.y, c.k);
  }
  sky->flush();
  nextGifMs = millis() + 60;
}

void animTick() {
  if ((int32_t)(millis() - nextGifMs) < 0) return;
  if (isBreak()) playZzzFrame(); else drawSky();
}

void drawAnimScreen() {
  int delayMs;
  if (isBreak()) fondGif.open((uint8_t *)CHAT_FOND, sizeof(CHAT_FOND), gifDraw);
  else fondGif.open((uint8_t *)TRAVAIL_FOND, sizeof(TRAVAIL_FOND), gifDraw);
  fondGif.playFrame(false, &delayMs, (void *)&FOND_ORIGIN);
  fondGif.close();
  zzzGif.reset();
  nextGifMs = millis();
  animTick();
  shownSeconds = -1;
  drawTime();
  drawCentered(phaseName(), animInfoX(), animLabelY(), 2, COL_DIM, COL_BG);
  drawCentered("Toucher : menu", animInfoX(), isBreak() ? 300 : 225, 1, COL_DIM, COL_BG);
}

void enterAnim() {
  screen = ANIM;
  drawAnimScreen();
}

void drawTime() {
  uint32_t remainingMs = phaseMs - min(elapsedMs, phaseMs);
  int secs = (remainingMs + 999) / 1000;  // arrondi supérieur : 25:00 au départ, 00:00 à la fin
  if (secs == shownSeconds) return;
  shownSeconds = secs;
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", secs / 60, secs % 60);
  if (screen == ANIM) drawCentered(buf, animInfoX(), animTimeY(), 4, COL_TEXT, COL_BG);
  else drawCentered(buf, CX, CY - 24, 6, COL_TEXT, COL_BG);
}

// Part du haut de l'anneau (décalage de 270°), sens horaire
void drawArcProgress() {
  float deg = 360.0f * min(elapsedMs, phaseMs) / phaseMs;
  if (deg - arcDrawnDeg < 0.5f) return;
  gfx->fillArc(CX, CY, R_OUT, R_IN, 270 + arcDrawnDeg, 270 + deg, COL_TRACK);
  arcDrawnDeg = deg;
}

void drawMainScreen() {
  gfx->fillScreen(COL_BG);
  gfx->fillArc(CX, CY, R_OUT, R_IN, 0, 360, phaseColor());
  arcDrawnDeg = 0;
  shownSeconds = -1;
  drawArcProgress();
  drawCentered(phaseName(), CX, CY - 75, 2, COL_TEXT, COL_BG);
  drawTime();
  drawCycleDots();
  drawTotal();
  drawStartButton();
  drawButton(BTN_STOP, "STOP", COL_BTN);
  drawButton(BTN_SKIP, "PASSER", COL_BTN);
  drawButton(BTN_SETTINGS, "REGLAGES", COL_BTN);
}

void drawSettingValue(int i) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%d %s", settings[i], SETTING_UNITS[i]);
  gfx->fillRect(306, rowY(i), 108, 45, COL_BG);
  drawCentered(buf, 360, rowY(i) + 15, 2, COL_TEXT, COL_BG);
}

void drawSettingsScreen() {
  gfx->fillScreen(COL_BG);
  drawCentered("REGLAGES", 240, 15, 3, COL_TEXT, COL_BG);
  for (int i = 0; i < SETTING_COUNT; i++) {
    gfx->setTextSize(2);
    gfx->setTextColor(COL_TEXT, COL_BG);
    gfx->setCursor(10, rowY(i) + 15);
    gfx->print(SETTING_NAMES[i]);
    drawButton(minusBtn(i), "-", COL_BTN, 3);
    drawButton(plusBtn(i), "+", COL_BTN, 3);
    drawSettingValue(i);
  }
  drawButton(BTN_CALIBRATE, "CALIBRER", COL_BTN);
  drawButton(BTN_OK, "OK", COL_WORK);
}
