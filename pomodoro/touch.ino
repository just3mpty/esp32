// Lit le point touché en coordonnées écran (après calibrage). Renvoie false si rien n'est touché.
bool readTouch(int &x, int &y) {
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  float rx = cal.swap ? p.y : p.x;
  float ry = cal.swap ? p.x : p.y;
  x = constrain((int)(cal.ax * rx + cal.bx), 0, gfx->width() - 1);
  y = constrain((int)(cal.ay * ry + cal.by), 0, gfx->height() - 1);
  return true;
}

// Attend un appui sur une cible et renvoie la moyenne des valeurs brutes
TS_Point waitRawTouch(int tx, int ty) {
  gfx->fillScreen(COL_BG);
  drawCentered("Touche le centre de la croix", 240, 150, 2, COL_TEXT, COL_BG);
  gfx->drawFastHLine(tx - 15, ty, 31, COL_TEXT);
  gfx->drawFastVLine(tx, ty - 15, 31, COL_TEXT);
  gfx->drawCircle(tx, ty, 8, COL_TEXT);

  while (ts.touched()) delay(10);  // attendre que le doigt précédent soit levé
  delay(200);
  while (!ts.touched()) delay(10);

  long sx = 0, sy = 0;
  int n = 0;
  uint32_t t0 = millis();
  while (ts.touched() || millis() - t0 < 300) {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      sx += p.x; sy += p.y; n++;
    }
    delay(10);
  }
  TS_Point r(n ? sx / n : 0, n ? sy / n : 0, 0);
  Serial.printf("Cible (%d,%d) -> brut (%d,%d)\n", tx, ty, r.x, r.y);
  return r;
}

// Calibrage avec 3 cibles : la 2e est décalée en X, la 3e en Y.
// On en déduit quel axe brut correspond à X écran, et l'échelle de chaque axe.
void calibrate() {
  while (true) {
    TS_Point p1 = waitRawTouch(40, 40);
    TS_Point p2 = waitRawTouch(440, 40);
    TS_Point p3 = waitRawTouch(40, 280);

    cal.swap = abs(p2.y - p1.y) > abs(p2.x - p1.x);
    float dx = cal.swap ? p2.y - p1.y : p2.x - p1.x;
    float dy = cal.swap ? p3.x - p1.x : p3.y - p1.y;
    if (fabsf(dx) < 200 || fabsf(dy) < 200) {
      Serial.println("Calibrage incoherent, on recommence");
      continue;
    }
    cal.ax = 400 / dx;
    cal.bx = 40 - cal.ax * (cal.swap ? p1.y : p1.x);
    cal.ay = 240 / dy;
    cal.by = 40 - cal.ay * (cal.swap ? p1.x : p1.y);
    Serial.printf("Calibrage : swap=%d ax=%.4f bx=%.1f ay=%.4f by=%.1f\n", cal.swap, cal.ax, cal.bx, cal.ay, cal.by);
    savePrefs();
    while (ts.touched()) delay(10);
    return;
  }
}

// Traite un appui confirmé (position stable). Renvoie true si le bouton
// touché doit se répéter tant qu'on reste appuyé dessus.
bool handleTap(int x, int y) {
  if (screen == ANIM) {  // n'importe quel appui ramène les boutons
    screen = MAIN;
    drawMainScreen();
    return false;
  }
  if (screen == MAIN) {
    if (hit(BTN_START, x, y)) toggleRun();
    else if (hit(BTN_STOP, x, y)) { Serial.println("Stop"); startPhase(phase); }
    else if (hit(BTN_SKIP, x, y)) { Serial.println("Phase suivante"); nextPhase(); }
    else if (hit(BTN_SETTINGS, x, y)) { screen = SETTINGS; drawSettingsScreen(); }
    return false;
  }

  for (int i = 0; i < SETTING_COUNT; i++) {
    int delta = hit(minusBtn(i), x, y) ? -1 : hit(plusBtn(i), x, y) ? 1 : 0;
    if (!delta) continue;
    settings[i] = constrain(settings[i] + delta, SETTING_MIN[i], SETTING_MAX[i]);
    drawSettingValue(i);
    return true;
  }
  if (hit(BTN_CALIBRATE, x, y)) {
    calibrate();
    drawSettingsScreen();
  } else if (hit(BTN_OK, x, y)) {
    savePrefs();
    // Les nouvelles durées s'appliquent à la phase en cours
    phaseMs = phaseDurationMs(phase);
    if (doneInCycle > settings[3]) doneInCycle = settings[3];
    screen = MAIN;
    drawMainScreen();
  }
  return false;
}

void pollTouch() {
  // touching : contact brut en cours (peut n'être qu'un rebond ou du bruit)
  // down     : appui confirmé, pour lequel handleTap a déjà été appelé
  static bool touching = false, down = false, repeatable = false;
  static int touchX, touchY;  // position au début du contact courant (pour vérifier sa stabilité)
  static int lastX, lastY;    // position de l'appui confirmé (pour la répétition)
  static uint32_t touchStartMs, downAt, lastSeen, lastRepeat, lastHandledMs = 0;
  uint32_t now = millis();
  int x, y;

  if (readTouch(x, y)) {
    lastSeen = now;
    if (!touching) {
      touching = true;
      touchStartMs = now;
      touchX = x; touchY = y;
    } else if (abs(x - touchX) > TOUCH_JITTER_MAX || abs(y - touchY) > TOUCH_JITTER_MAX) {
      // Le point a trop bougé pour être le même contact (doigt qui glisse,
      // ou lecture parasite) : on redémarre la confirmation sur ce nouveau point.
      touchStartMs = now;
      touchX = x; touchY = y;
    }
    if (!down) {
      if (now - touchStartMs >= TOUCH_CONFIRM_MS && now - lastHandledMs >= TOUCH_COOLDOWN_MS) {
        down = true;
        lastX = touchX; lastY = touchY;
        lastTouchMs = now;
        downAt = lastRepeat = lastHandledMs = now;
        repeatable = handleTap(lastX, lastY);
      }
    } else if (repeatable && now - downAt > 500 && now - lastRepeat > 120) {
      lastRepeat = lastHandledMs = now;
      handleTap(lastX, lastY);
    }
  } else if (touching && now - lastSeen > 80) {
    touching = false;  // relâché (avec une petite marge contre les micro-coupures)
    down = false;
  }
}

// Bouton BOOT maintenu 1 s : recalibrage du tactile
void pollBootButton() {
  static uint32_t downAt = 0;
  if (digitalRead(BTN_PIN) == HIGH) { downAt = 0; return; }
  if (!downAt) downAt = millis();
  if (millis() - downAt >= 1000) {
    calibrate();
    downAt = 0;
    if (screen == MAIN) drawMainScreen();
    else if (screen == ANIM) drawAnimScreen();
    else drawSettingsScreen();
  }
}
