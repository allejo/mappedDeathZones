/*
 * Copyright (C) 2020 Vladimir "allejo" Jimenez
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "bzfsAPI.h"
#include "plugin_utils.h"

class DeathzoneZone : public bz_CustomZoneObject
{
public:
    DeathzoneZone() : bz_CustomZoneObject()
    {
    }

    bz_eTeamType team_value;
    std::string spawnzone_value;
    std::string name_value;
};

class SpawnzoneZone : public bz_CustomZoneObject
{
public:
    SpawnzoneZone() : bz_CustomZoneObject()
    {
    }

    std::string name_name;
};

class MappedDeathZones : public bz_Plugin, public bz_CustomMapObjectHandler
{
public:
    virtual const char* Name();
    virtual void Init(const char* config);
    virtual void Cleanup();
    virtual void Event(bz_EventData* eventData);
    virtual bool MapObject(bz_ApiString object, bz_CustomMapObjectInfo* data);
};

BZ_PLUGIN(MappedDeathZones)

const char* MappedDeathZones::Name()
{
    return "Mapped Death Zones";
}

void MappedDeathZones::Init(const char* config)
{
    Register(bz_ePlayerDieEvent);

    bz_registerCustomMapObject("DEATHZONE", this);
    bz_registerCustomMapObject("SPAWNZONE", this);
}

void MappedDeathZones::Cleanup()
{
    Flush();

    bz_removeCustomMapObject("DEATHZONE");
    bz_removeCustomMapObject("SPAWNZONE");
}

void MappedDeathZones::Event(bz_EventData* eventData)
{
    switch (eventData->eventType)
    {
        case bz_ePlayerDieEvent:
        {
            // This event is called each time a tank is killed.
            bz_PlayerDieEventData_V2* data = (bz_PlayerDieEventData_V2*)eventData;

            // Data
            // ----
            // (int)                  playerID           - ID of the player who was killed.
            // (bz_eTeamType)         team               - The team the killed player was on.
            // (int)                  killerID           - The owner of the shot that killed the player, or BZ_SERVER for server side kills
            // (bz_eTeamType)         killerTeam         - The team the owner of the shot was on.
            // (bz_ApiString)         flagKilledWith     - The flag name the owner of the shot had when the shot was fired.
            // (int)                  flagHeldWhenKilled - The ID of the flag the victim was holding when they died.
            // (int)                  shotID             - The shot ID that killed the player, if the player was not killed by a shot, the id will be -1.
            // (bz_PlayerUpdateState) state              - The state record for the killed player at the time of the event
            // (double)               eventTime          - Time of the event on the server.
        }
        break;

        default:
            break;
    }
}

bool MappedDeathZones::MapObject(bz_ApiString object, bz_CustomMapObjectInfo* data)
{
    // Note, this value will be in uppercase
    if (!data || object != "DEATHZONE" || object != "SPAWNZONE")
    {
        return false;
    }

    if (object == "DEATHZONE")
    {
        DeathzoneZone deathzoneZone;
        deathzoneZone.handleDefaultOptions(data);

        for (unsigned int i = 0; i < data->data.size(); i++)
        {
            std::string line = data->data.get(i);

            bz_APIStringList nubs;
            nubs.tokenize(line.c_str(), " ", 0, true);

            if (nubs.size() > 0)
            {
                std::string key = bz_toupper(nubs.get(0).c_str());

                if (key == "TEAM")
                {
                    deathzoneZone.team_value = (bz_eTeamType)atoi(nubs.get(1).c_str());
                }
                else if (key == "SPAWNZONE")
                {
                    deathzoneZone.spawnzone_value = nubs.get(1).c_str();
                }
                else if (key == "NAME")
                {
                    deathzoneZone.name_value = nubs.get(1).c_str();
                }
            }
        }
    }
    else if (object == "SPAWNZONE")
    {
        SpawnzoneZone spawnzoneZone;
        spawnzoneZone.handleDefaultOptions(data);

        for (unsigned int i = 0; i < data->data.size(); i++)
        {
            std::string line = data->data.get(i);

            bz_APIStringList nubs;
            nubs.tokenize(line.c_str(), " ", 0, true);

            if (nubs.size() > 0)
            {
                std::string key = bz_toupper(nubs.get(0).c_str());

                if (key == "NAME")
                {
                    spawnzoneZone.name_name = nubs.get(1).c_str();
                }
            }
        }
    }

    // @TODO Save your custom map objects to your class

    return true;
}