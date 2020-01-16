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

class DeathZone : public bz_CustomZoneObject
{
public:
    DeathZone() : bz_CustomZoneObject()
    {
    }

    std::string name;
    std::vector<bz_eTeamType> affectedTeams;
    std::vector<std::string> spawnZones;

    bool doesAffectTeam(bz_eTeamType team)
    {
        if (affectedTeams.size() == 0)
        {
            return true;
        }

        return std::find(affectedTeams.begin(), affectedTeams.end(), team) != affectedTeams.end();
    }

    std::string getRandomSpawnZone()
    {
        if (spawnZones.size() == 0)
        {
            return "";
        }

        int target = rand() % spawnZones.size();

        return spawnZones.at(target);
    }
};

class SpawnZone : public bz_CustomZoneObject
{
public:
    SpawnZone() : bz_CustomZoneObject()
    {
    }

    std::string name;
};

class MappedDeathZones : public bz_Plugin, public bz_CustomMapObjectHandler
{
public:
    virtual const char* Name();
    virtual void Init(const char* config);
    virtual void Cleanup();
    virtual void Event(bz_EventData* eventData);
    virtual bool MapObject(bz_ApiString object, bz_CustomMapObjectInfo* data);

private:
    std::map<std::string, DeathZone> deathZones;
    std::map<std::string, SpawnZone> spawnZones;

    std::map<int, std::string> nextSpawnZone;
};

BZ_PLUGIN(MappedDeathZones)

const char* MappedDeathZones::Name()
{
    return "Mapped Death Zones";
}

void MappedDeathZones::Init(const char* config)
{
    Register(bz_eGetPlayerSpawnPosEvent);
    Register(bz_ePlayerDieEvent);
    Register(bz_eWorldFinalized);

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
        case bz_eGetPlayerSpawnPosEvent:
        {
            bz_GetPlayerSpawnPosEventData_V1* data = (bz_GetPlayerSpawnPosEventData_V1*)eventData;

            if (nextSpawnZone.find(data->playerID) != nextSpawnZone.end())
            {
                std::string target = nextSpawnZone[data->playerID];

                if (target.size() == 0)
                {
                    return;
                }

                if (spawnZones.find(target) == spawnZones.end())
                {
                    return;
                }

                float spawnPos[3];
                bz_getSpawnPointWithin(&spawnZones[target], spawnPos);

                data->handled = true;
                data->pos[0] = spawnPos[0];
                data->pos[1] = spawnPos[1];
                data->pos[2] = spawnPos[2];

                nextSpawnZone.erase(data->playerID);
            }
        }
        break;

        case bz_ePlayerDieEvent:
        {
            // This event is called each time a tank is killed.
            bz_PlayerDieEventData_V2* data = (bz_PlayerDieEventData_V2*)eventData;


            for (auto &deathZone : deathZones)
            {
                if (
                    deathZone.second.doesAffectTeam(data->team) &&
                    deathZone.second.pointInZone(data->state.pos)
                ) {
                    nextSpawnZone[data->playerID] = deathZone.second.getRandomSpawnZone();

                    break;
                }
            }
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
        DeathZone deathZone;
        deathZone.handleDefaultOptions(data);

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
                    deathZone.affectedTeams.push_back((bz_eTeamType)atoi(nubs.get(1).c_str()));
                }
                else if (key == "SPAWNZONE")
                {
                    deathZone.spawnZones.push_back(nubs.get(1).c_str());
                }
                else if (key == "NAME")
                {
                    deathZone.name = nubs.get(1).c_str();
                }
            }
        }

        deathZones[deathZone.name] = deathZone;
    }
    else if (object == "SPAWNZONE")
    {
        SpawnZone spawnZone;
        spawnZone.handleDefaultOptions(data);

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
                    spawnZone.name = nubs.get(1).c_str();
                }
            }
        }

        spawnZones[spawnZone.name] = spawnZone;
    }

    return true;
}
