#include "gaming-companion-dock.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/bmem.h>

#include <QMetaObject>
#include <QString>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-gaming-companion", "de-DE")

static GamingCompanionDock *g_dock = nullptr;
static obs_hotkey_id g_short_hotkey = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id g_kill_hotkey = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id g_highlight_hotkey = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id g_funny_hotkey = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id g_bug_hotkey = OBS_INVALID_HOTKEY_ID;

static void invoke_marker(const char *name)
{
    if (!g_dock)
        return;
    const QString marker = QString::fromUtf8(name);
    QMetaObject::invokeMethod(g_dock, [marker]() {
        if (g_dock)
            g_dock->addMarker(marker);
    }, Qt::QueuedConnection);
}

static void short_hotkey(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
    if (pressed && g_dock)
        QMetaObject::invokeMethod(g_dock, []() { if (g_dock) g_dock->saveShort(); }, Qt::QueuedConnection);
}
static void kill_hotkey(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) { if (pressed) invoke_marker("Kill"); }
static void highlight_hotkey(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) { if (pressed) invoke_marker("Highlight"); }
static void funny_hotkey(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) { if (pressed) invoke_marker("Lustig"); }
static void bug_hotkey(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) { if (pressed) invoke_marker("Bug"); }

static void frontend_event(enum obs_frontend_event event, void *)
{
    if (!g_dock)
        return;

    if (event == OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED) {
        char *path = obs_frontend_get_last_replay();
        if (path && *path) {
            const QString replay = QString::fromUtf8(path);
            QMetaObject::invokeMethod(g_dock, [replay]() {
                if (g_dock)
                    g_dock->onReplaySaved(replay);
            }, Qt::QueuedConnection);
        }
        bfree(path);
    }
}

bool obs_module_load(void)
{
    g_dock = new GamingCompanionDock();
    if (!obs_frontend_add_dock_by_id("obs-gaming-companion", "OBS Gaming Companion", g_dock)) {
        blog(LOG_ERROR, "[OBS Gaming Companion] Dock konnte nicht hinzugefuegt werden");
        delete g_dock;
        g_dock = nullptr;
        return false;
    }

    g_short_hotkey = obs_hotkey_register_frontend("gaming_companion.save_short", "OBS Gaming Companion: SHORT + CLIP speichern", short_hotkey, nullptr);
    g_kill_hotkey = obs_hotkey_register_frontend("gaming_companion.marker_kill", "OBS Gaming Companion: Marker Kill", kill_hotkey, nullptr);
    g_highlight_hotkey = obs_hotkey_register_frontend("gaming_companion.marker_highlight", "OBS Gaming Companion: Marker Highlight", highlight_hotkey, nullptr);
    g_funny_hotkey = obs_hotkey_register_frontend("gaming_companion.marker_funny", "OBS Gaming Companion: Marker Lustig", funny_hotkey, nullptr);
    g_bug_hotkey = obs_hotkey_register_frontend("gaming_companion.marker_bug", "OBS Gaming Companion: Marker Bug", bug_hotkey, nullptr);

    obs_frontend_add_event_callback(frontend_event, nullptr);
    blog(LOG_INFO, "[OBS Gaming Companion] v0.5.1 geladen");
    return true;
}

void obs_module_unload(void)
{
    obs_frontend_remove_event_callback(frontend_event, nullptr);
    obs_frontend_remove_dock("obs-gaming-companion");
    g_dock = nullptr;
    blog(LOG_INFO, "[OBS Gaming Companion] entladen");
}

MODULE_EXPORT const char *obs_module_description(void)
{
    return "Gaming-Dock fuer OBS mit Replay-Puffer, 16:9 Clips, 9:16 Shorts, Webcam-Layout, Logo, lokaler Whisper-Untertitelung, Auto-Highlight-Kandidaten, Markern und Windows-Installer-Support.";
}
