#pragma once
#include <string>
#include <map>
#include <vector>
#include <soloud.h>
#include <soloud_wav.h>

class audio
{
public:

	// must be called before playing sounds
	static void init();
	static void deinit();

	// unlike voice groups, sounds are not automatically created.
	// they must be manually loaded here.
	// `sound` is a filename.
	static bool loadSound(const std::string& sound);

	struct Sound4D
	{
		SoLoud::handle XYZ = 0;
		SoLoud::handle XYW = 0;
	};

	static SoLoud::handle playSound(const std::string& sound, const std::string& voiceGroup);

	// NOTE: the sound must already be loaded
	static void addToBgmList(const std::string& sound);
	// this method automatically loads background music and adds it to the BGM queue.
	// returns true if at least one track is successfully loaded.
	static bool loadBgmfromJson(const std::string& jsonListPath);
	static void clearBgm();

	// set volume from 0-1
	static void setGlobalVolume(float volume);
	static void setVoiceGroupVolume(const std::string& voiceGroup, float volume);
	static void setBgmVolume(float volume);

	static SoLoud::handle getVoiceGroupHandle(const std::string& voiceGroup);

	// plays the next song in the BGM list if applicable.
	// must call init() before using this function.
	// songs do not get removed from the list after being played.
	// will loop to the beginning of the list once it has reached the end.
	// returns false if no music is playing and there are no playable tracks in the list.
	static bool updateBgm();

	// stops playing the current background music.
	// subsequent calls to updateBGM() will start up the next song rather than resuming the current one.
	// returns false if no background music is playing when called.
	static bool stopBgm();

private:

	struct voiceGroupInfo
	{
		SoLoud::handle handle;
		float volume;
	};

	static SoLoud::Wav* getSound(const std::string& sound);

	static voiceGroupInfo* getVoiceGroupInfo(const std::string& voiceGroup);

	// soloud
	static SoLoud::Soloud soloud;

	static std::map<std::string, voiceGroupInfo> voiceGroups;

	static std::map<std::string, SoLoud::Wav> sounds;

	static std::vector<std::string> bgmList;
	// set to 1 by default
	static float bgmVolume;
	// index of the next song to be played. starts out at 0.
	static int bgmNextIndex;
	// starts out at 0
	static SoLoud::handle bgmCurrent;
};