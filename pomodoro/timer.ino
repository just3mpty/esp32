void startPhase(Phase p) {
  phase = p;
  phaseMs = phaseDurationMs(p);
  elapsedMs = 0;
  state = READY;
  Serial.printf("Phase : %s (%lu s)\n", phaseName(), phaseMs / 1000);
  screen = MAIN;
  drawMainScreen();
}

void nextPhase() {
  if (phase == WORK) {
    doneInCycle++;
    doneTotal++;
    startPhase(doneInCycle >= settings[3] ? LONG_BREAK : SHORT_BREAK);
  } else {
    if (phase == LONG_BREAK) doneInCycle = 0;
    startPhase(WORK);
  }
}

void toggleRun() {
  if (state == RUNNING) {
    state = PAUSED;
    Serial.println("Pause");
  } else {
    state = RUNNING;
    runStartMs = millis();
    elapsedAtStart = elapsedMs;
    Serial.println(elapsedMs ? "Reprise" : "Demarrage");
    enterAnim();
    return;
  }
  drawStartButton();
}

void flashEnd() {
  uint16_t c = phaseColor();
  for (int i = 0; i < 3; i++) {
    gfx->fillScreen(c);
    beep(1);
    delay(200);
    gfx->fillScreen(COL_BG);
    delay(200);
  }
}
