#ifndef SETTINGS_H
#define SETTINGS_H

struct Settings {
    bool showDescriptions = true;
    bool showCompletedTasks = true;
};

Settings loadSettings();
void saveSettings(const Settings& s);

#endif
