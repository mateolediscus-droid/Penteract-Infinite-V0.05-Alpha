#include "audio.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// static members
SoLoud::Soloud											audio::soloud;
std::map<std::string, audio::voiceGroupInfo>		audio::voiceGroups;
std::map<std::string, SoLoud::Wav>						audio::sounds;
std::vector<std::string>								audio::bgmList;
float													audio::bgmVolume = 1;
int														audio::bgmNextIndex = 0;
SoLoud::handle											audio::bgmCurrent = 0;

void audio::init()
{
	soloud.init();
}

void audio::deinit()
{
	soloud.deinit();
}

bool audio::loadSound(const std::string& sound)
{
	auto insertion = sounds.emplace(std::make_pair(sound, SoLoud::Wav()));

	if (!insertion.second)
	{
		return false;
	}

	SoLoud::Wav& sample = insertion.first->second;

	std::string samplePath = "assets/audio/";
	samplePath.append(sound);

	if (sample.load(samplePath.c_str()) != 0)
	{
		// remove the sample from the map if it couldn't load
		sounds.erase(insertion.first);

		return false;
	}

	return true;
}

SoLoud::handle audio::playSound(const std::string& sound, const std::string& voiceGroup)
{
	SoLoud::Wav* s = getSound(sound);

	if (s == nullptr)
	{
		return 0;
	}

	SoLoud::handle h = soloud.play(*s);

	voiceGroupInfo* vgi = getVoiceGroupInfo(voiceGroup);

	soloud.setVolume(h, vgi->volume);

	soloud.addVoiceToGroup(vgi->handle, h);

	return h;
}

void audio::addToBgmList(const std::string& sound)
{
	bgmList.push_back(sound);
}

bool audio::loadBgmfromJson(const std::string& jsonListPath)
{
	// read a JSON file
	json j;
	try
	{
		std::ifstream i(jsonListPath);
		i >> j;
	}
	catch (...)
	{
		std::cout << "error: unable to load track list file \"" << jsonListPath << "\"\n";
		return false;
	}

	if (!j.is_array())
	{
		std::cout << "error: expected an array for track list\n";
		return false;
	}

	// return true if at least one track is successfully loaded.
	bool result = false;
	for (auto& item : j)
	{
		if (!item.is_string())
		{
			std::cout << "error: all track names must be strings\n";
			continue;
		}

		std::string trackName = item.get<std::string>();

		if (!loadSound(trackName))
		{
			std::cout << "error: unable to load track \"" << trackName << "\"\n";
		}
		else
		{
			addToBgmList(trackName);
			std::cout << "successfully loaded track \"" << trackName << "\"\n";
		}

		result = true;
	}

	return result;
}

void audio::clearBgm()
{
	stopBgm();
	bgmList.clear();
	bgmNextIndex = 0;
}

void audio::setGlobalVolume(float volume)
{
	soloud.setGlobalVolume(volume);
}

void audio::setVoiceGroupVolume(const std::string& voiceGroup, float volume)
{
	voiceGroupInfo* vgi = getVoiceGroupInfo(voiceGroup);
	vgi->volume = volume;

	soloud.setVolume(vgi->handle, volume);
}

void audio::setBgmVolume(float volume)
{
	bgmVolume = volume;

	if (soloud.isValidVoiceHandle(bgmCurrent))
	{
		soloud.setVolume(bgmCurrent, volume);
	}
}

SoLoud::handle audio::getVoiceGroupHandle(const std::string& voiceGroup)
{
	return getVoiceGroupInfo(voiceGroup)->handle;
}

bool audio::updateBgm()
{
	if (soloud.isValidVoiceHandle(bgmCurrent))
	{
		// return true if the current track is still playing
		return true;
	}

	if (bgmList.empty())
	{
		return false;
	}

	// loop through the list until a playable song is found.
	// return false if no playable songs are found.
	int i = bgmNextIndex;
	for (;;)
	{
		// try to play the current track
		SoLoud::Wav* currentTrack = getSound(bgmList[i]);
		if (currentTrack != nullptr)
		{
			bgmCurrent = soloud.play(*currentTrack, bgmVolume);
			if (bgmCurrent)
			{
				// set BGMNextIndex to the next song
				bgmNextIndex = i + 1;
				if (bgmNextIndex == bgmList.size())
				{
					bgmNextIndex = 0;
				}

				// prevent the BGM voice from being killed
				soloud.setProtectVoice(bgmCurrent, true);

				// return true if the current track is successfully started
				return true;
			}
		}

		// advance to the next track
		++i;
		if (i == bgmList.size())
		{
			i = 0;
		}

		if (i == bgmNextIndex)
		{
			// if no playable tracks were found, including the previous track, return false
			return false;
		}
	}
}

bool audio::stopBgm()
{
	if (bgmList.empty() || !soloud.isValidVoiceHandle(bgmCurrent))
	{
		return false;
	}

	soloud.stop(bgmCurrent);
	return true;
}

SoLoud::Wav* audio::getSound(const std::string& sound)
{
	auto search = sounds.find(sound);
	if (search == sounds.end())
	{
		return nullptr;
	}
	return &search->second;
}

audio::voiceGroupInfo* audio::getVoiceGroupInfo(const std::string& voiceGroup)
{
	auto search = voiceGroups.find(voiceGroup);
	if (search == voiceGroups.end())
	{
		// create a new voice group
		SoLoud::handle h = soloud.createVoiceGroup();
		auto insertion = voiceGroups.insert({ voiceGroup, { h, 1.0f } });
		return &insertion.first->second;
	}
	else
	{
		return &search->second;
	}
}