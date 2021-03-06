/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SETTINGS_MANAGER_H
#define _SETTINGS_MANAGER_H

#include <common.h> // for wxString hash
#include <settings/color_settings.h>

class COLOR_SETTINGS;
class COMMON_SETTINGS;


class SETTINGS_MANAGER
{
public:
    SETTINGS_MANAGER();

    ~SETTINGS_MANAGER();

    /**
     * @return true if settings load was successful
     */
    bool IsOK() { return m_ok; }

    /**
     * Takes ownership of the pointer passed in
     * @param aSettings is a settings object to register
     * @return a handle to the owned pointer
     */
    JSON_SETTINGS* RegisterSettings( JSON_SETTINGS* aSettings, bool aLoadNow = true );

    void Load();

    void Load( JSON_SETTINGS* aSettings );

    void Save();

    void Save( JSON_SETTINGS* aSettings );

    /**
     * If the given settings object is registered, save it to disk and unregister it
     * @param aSettings is the object to release
     */
    void FlushAndRelease( JSON_SETTINGS* aSettings );

    /**
     * Returns a handle to the a given settings by type
     * If the settings have already been loaded, returns the existing pointer.
     * If the settings have not been loaded, creates a new object owned by the
     * settings manager and returns a pointer to it.
     *
     * @tparam AppSettings is a type derived from APP_SETTINGS_BASE
     * @param aLoadNow is true to load the registered file from disk immediately
     * @return a pointer to a loaded settings object
     */
    template<typename AppSettings>
    AppSettings* GetAppSettings( bool aLoadNow = true )
    {
        AppSettings* ret = nullptr;

        auto it = std::find_if( m_settings.begin(), m_settings.end(),
                                []( const std::unique_ptr<JSON_SETTINGS>& aSettings ) {
                                    return dynamic_cast<AppSettings*>( aSettings.get() );
                                } );

        if( it != m_settings.end() )
            ret = dynamic_cast<AppSettings*>( it->get() );
        else
            ret = static_cast<AppSettings*>( RegisterSettings( new AppSettings, aLoadNow ) );

        return ret;
    }

    /**
     * Retrieves a color settings object that applications can read colors from.
     * If the given settings file cannot be found, returns the default settings.
     *
     * @param aName is the name of the color scheme to load
     * @return a loaded COLOR_SETTINGS object
     */
    COLOR_SETTINGS* GetColorSettings( const wxString& aName = "user" );

    std::vector<COLOR_SETTINGS*> GetColorSettingsList()
    {
        std::vector<COLOR_SETTINGS*> ret;

        for( const auto& el : m_color_settings )
            ret.push_back( el.second );

        std::sort( ret.begin(), ret.end(), []( COLOR_SETTINGS* a, COLOR_SETTINGS* b ) {
            return a->GetName() < b->GetName();
        } );

        return ret;
    }

    /**
     * Safely saves a COLOR_SETTINGS to disk, preserving any changes outside the given namespace.
     *
     * A color settings namespace is one of the top-level JSON objects like "board", etc.
     * This will preform a read-modify-write
     *
     * @param aSettings is a pointer to a valid COLOR_SETTINGS object managed by SETTINGS_MANAGER
     * @param aNamespace is the namespace of settings to save
     */
    void SaveColorSettings( COLOR_SETTINGS* aSettings, const std::string& aNamespace = "" );

    /**
     * Registers a new color settings object with the given filename
     * @param aFilename is the location to store the new settings object
     * @return a pointer to the new object
     */
    COLOR_SETTINGS* AddNewColorSettings( const wxString& aFilename );

    /**
     * Retrieves the common settings shared by all applications
     * @return a pointer to a loaded COMMON_SETTINGS
     */
    COMMON_SETTINGS* GetCommonSettings() { return m_common_settings; }

    /**
     * Returns the path a given settings file should be loaded from / stored to.
     * @param aSettings is the settings object
     * @return a path based on aSettings->m_location
     */
    std::string GetPathForSettingsFile( JSON_SETTINGS* aSettings );

    /**
     * Handles the initialization of the user settings directory and migration from previous
     * KiCad versions as needed.
     *
     * This method will check for the existence of the user settings path for this KiCad version.
     * If it exists, settings load will proceed normally using that path.
     *
     * If that directory is empty or does not exist, the migration wizard will be launched, which
     * will give users the option to migrate settings from a previous KiCad version (if one is
     * found), manually specify a directory to migrate fromm, or start with default settings.
     *
     * @return true if migration was successful or not necessary, false otherwise
     */
    bool MigrateIfNeeded();

    /**
     * Helper for DIALOG_MIGRATE_SETTINGS to specify a source for migration
     * @param aSource is a directory containing settings files to migrate from (can be empty)
     */
    void SetMigrationSource( const wxString& aSource ) { m_migration_source = aSource; }

    /**
     * Retreives the name of the most recent previous KiCad version that can be found in the
     * user settings directory.  For legacy versions (5.x, and 5.99 builds before this code was
     * written), this will return "5.x"
     *
     * @param aName is filled with the name of the previous version, if one exists
     * @return true if a previous version to migrate from exists
     */
    bool GetPreviousVersionPaths( std::vector<wxString>* aName = nullptr );

    /**
     * Checks if a given path is probably a valid KiCad configuration directory.
     * Actually it just checks if a file called "kicad_common" exists, because that's probably
     * good enough for now.
     *
     * @param aPath is the path to check
     * @return true if the path contains KiCad settings
     */
    static bool IsSettingsPathValid( const wxString& aPath );

    /**
     * Returns the path where color scheme files are stored
     * (normally ./colors/ under the user settings path)
     */
    static std::string GetColorSettingsPath();

    /**
     * Return the user configuration path used to store KiCad's configuration files.
     *
     * @see calculateUserSettingsPath
     *
     * NOTE: The path is cached at startup, it will never change during program lifetime!
     *
     * @return A string containing the config path for Kicad
     */
    static std::string GetUserSettingsPath();

    /**
     * Parses the current KiCad build version and extracts the major and minor revision to use
     * as the name of the settings directory for this KiCad version.
     *
     * @return a string such as "5.1"
     */
    static std::string GetSettingsVersion();

private:

    /**
     * Determines the base path for user settings files.
     *
     * The configuration path order of precedence is determined by the following criteria:
     *
     * - The value of the KICAD_CONFIG_HOME environment variable
     * - The value of the XDG_CONFIG_HOME environment variable.
     * - The result of the call to wxStandardPaths::GetUserConfigDir() with ".config" appended
     *   as required on Linux builds.
     *
     * @param aIncludeVer will append the current KiCad version if true (default)
     * @param aUseEnv will prefer the base path found in the KICAD_CONFIG_DIR if found (default)
     * @return A string containing the config path for Kicad
     */
    static std::string calculateUserSettingsPath( bool aIncludeVer = true, bool aUseEnv = true );

    /**
     * Compares two settings versions, like "5.99" and "6.0"
     * @return -1 if aFirst is older than aSecond, 1 if aFirst is newer than aSecond, 0 otherwise
     */
    static int compareVersions( const std::string& aFirst, const std::string& aSecond );

    /**
     * Extracts the numeric version from a given settings string
     * @param aVersionString is the string to split at the "."
     * @param aMajor will store the first part
     * @param aMinor will store the second part
     * @return true if extraction succeeded
     */
    static bool extractVersion( const std::string& aVersionString, int* aMajor, int* aMinor );

    /**
     * Attempts to load a color theme by name (the color theme directory and .json ext are assumed)
     * @param aName is the filename of the color theme (without the extension or path)
     * @return the loaded settings, or nullptr if load failed
     */
    COLOR_SETTINGS* loadColorSettingsByName( const wxString& aName );

    void registerColorSettings( const wxString& aFilename );

    void loadAllColorSettings();

    std::vector<std::unique_ptr<JSON_SETTINGS>> m_settings;

    std::unordered_map<wxString, COLOR_SETTINGS*> m_color_settings;

    // Convenience shortcut
    COMMON_SETTINGS* m_common_settings;

    wxString m_migration_source;

    /// True if settings loaded successfully at construction
    bool m_ok;
};

#endif
