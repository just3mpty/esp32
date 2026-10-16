void loadPrefs() {
  prefs.begin("pomodoro", true);
  prefs.getBytes("settings", settings, sizeof(settings));
  bool hasCal = prefs.getBytes("cal", &cal, sizeof(cal)) == sizeof(cal);
  prefs.end();
  if (!hasCal) cal.ax = 0;  // ax == 0 : pas de calibrage enregistré
}

void savePrefs() {
  prefs.begin("pomodoro", false);
  prefs.putBytes("settings", settings, sizeof(settings));
  prefs.putBytes("cal", &cal, sizeof(cal));
  prefs.end();
}
