#include "script.h"

#include "level.h"

std::string FindInstanceName(file_t& file, addr_t instAddr)
{
	if (file.ReadAt<addr_t>(instAddr) == 0)
		return "";

	file.seek(file.ReadAt<addr_t>(instAddr));
	file.seek(file.Read<addr_t>(0x24), true);
	std::string str(8, '_');
	memcpy(str.data(), file.ptr<char>(), 8);
	file.pop();
	return str;
}

void ParseCommands(level_t& level, file_t& file, addr_t address, std::vector<std::string>& commands)
{
	// Script command can have the 0x80000000 flag set?
	if (address == 0)
		return;

	file.seek(address);
	unsigned int sz = file.Read<unsigned int>(0, true); // count

	static const std::vector<const char*> scripts = {
		"hideObject", "unhideObject", "timer", "gotoFrame", "changeModel", "startAniTex", "stopAniTex", "startSpline", "stopSpline", "deathZ", "dsignal", "gsignal", "lightGroup", "cameraAdjust", "cameraMode", "camera", "cameraTimer", "cameraSmooth", "cameraValue??", "cameraLock", "cameraUnlock", "cameraSave", "cameraRestore", "teleport", "farPlane", "soundStartSequence", "soundStopSlot", "soundPauseSlot", "soundResumeSlot", "soundMuteChannel", "soundUnmuteChannel", "times", "freeze", "unfreeze", "freezeAll", "unfreezeAll", "hideBG", "unhideBG", "hideBGObject", "unhideBGObject", "mirror", "unmirror", "setMirror", "fogNear", "fogFar", "startVertexMorph", "stopVertexMorph", "logicValue", "cameraShake", "logicAnd", "logicOr", "logicXor", "logicTrue", "logicFalse", "callSignal", "offset", "logicAdd", "logicSub", "goto", "label", "end", "gosub", "stopPlayerControl", "startPlayerControl", "setPlayerControl", "launch", "costumeChange", "gameValue", "setZSignal", "resetZSignal", "SoundEffect", "MusicControl", "LevelChange", "VoiceControl", "BankChange", "GameLoadSave", "HideObjectGroup", "UnhideObjectGroup", "Shards", "cameraSpline??", "screenWipe", "VoiceQueue", "VoiceUnQueue", "VoiceRequest", "VoiceClearQueue", "VoiceForce", "setMusicVariable", "introActive", "introFX", "gotoPos", "frame", "birthObject", "blendStart", "miscValue", "VoiceDisable", "VoiceReEnable", "setTimes", "screenWipeColor", "relocate", "logicTrueElse", "logicFalseElse", "print", "tagTimer", "setNoRemove", "resetNoRemove"
	};
	enum class EScriptType : unsigned int
	{
		hideObject, unhideObject, timer, gotoFrame, changeModel, startAniTex,
		stopAniTex, startSpline, stopSpline, deathZ, dsignal, gsignal, lightGroup,
		cameraAdjust, cameraMode, camera, cameraTimer, cameraSmooth, cameraValueQQ,
		cameraLock, cameraUnlock, cameraSave, cameraRestore, teleport, farPlane,
		soundStartSequence, soundStopSlot, soundPauseSlot, soundResumeSlot,
		soundMuteChannel, soundUnmuteChannel, times, freeze, unfreeze, freezeAll,
		unfreezeAll, hideBG, unhideBG, hideBGObject, unhideBGObject, mirror, unmirror,
		setMirror, fogNear, fogFar, startVertexMorph, stopVertexMorph, logicValue,
		cameraShake, logicAnd, logicOr, logicXor, logicTrue, logicFalse, callSignal,
		offset, logicAdd, logicSub, _goto, label, end, gosub, stopPlayerControl,
		startPlayerControl, setPlayerControl, launch, costumeChange, gameValue,
		setZSignal, resetZSignal, SoundEffect, MusicControl, LevelChange, VoiceControl,
		BankChange, GameLoadSave, HideObjectGroup, UnhideObjectGroup, Shards,
		cameraSplineQQ, screenWipe, VoiceQueue, VoiceUnQueue, VoiceRequest,
		VoiceClearQueue, VoiceForce, setMusicVariable, introActive, introFX,
		gotoPos, frame, birthObject, blendStart, miscValue, VoiceDisable,
		VoiceReEnable, setTimes, screenWipeColor, relocate, logicTrueElse,
		logicFalseElse, print, tagTimer, setNoRemove, resetNoRemove
	};
	/*
		00: hideObject
		01: unhideObject
		02: timer
		03: gotoFrame
		04: changeModel
		05: startAniTex
		06: stopAniTex
		07: startSpline
		08: stopSpline
		09: deathZ
		0A: dsignal
		0B: gsignal
		0C: lightGroup
		0D: cameraAdjust
		0E: cameraMode
		0F: camera
		10: cameraTimer
		11: cameraSmooth
		12: cameraValue??
		13: cameraLock
		14: cameraUnlock
		15: cameraSave
		16: cameraRestore
		17: teleport
		18: farPlane
		19: soundStartSequence
		1A: soundStopSlot
		1B: soundPauseSlot
		1C: soundResumeSlot
		1D: soundMuteChannel
		1E: soundUnmuteChannel
		1F: times
		20: freeze
		21: unfreeze
		22: freezeAll
		23: unfreezeAll
		24: hideBG
		25: unhideBG
		26: hideBGObject
		27: unhideBGObject
		28: mirror
		29: unmirror
		2A: setMirror
		2B: fogNear
		2C: fogFar
		2D: startVertexMorph
		2E: stopVertexMorph
		2F: logicValue
		30: cameraShake
		31: logicAnd
		32: logicOr
		33: logicXor
		34: logicTrue
		35: logicFalse
		36: callSignal
		37: offset
		38: logicAdd
		39: logicSub
		3A: goto
		3B: label
		3C: end
		3D: gosub
		3E: stopPlayerControl
		3F: startPlayerControl
		40: setPlayerControl
		41: launch
		42: costumeChange
		43: gameValue
		44: setZSignal
		45: resetZSignal
		46: SoundEffect
		47: MusicControl
		48: LevelChange
		49: VoiceControl
		4A: BankChange
		4B: GameLoadSave
		4C: HideObjectGroup
		4D: UnhideObjectGroup
		4E: Shards
		4F: cameraSpline??
		50: screenWipe
		51: VoiceQueue
		52: VoiceUnQueue
		53: VoiceRequest
		54: VoiceClearQueue
		55: VoiceForce
		56: setMusicVariable
		57: introActive
		58: introFX
		59: gotoPos
		5A: frame
		5B: birthObject
		5C: blendStart
		5D: miscValue
		5E: VoiceDisable
		5F: VoiceReEnable
		60: setTimes
		61: screenWipeColor
		62: relocate
		63: logicTrueElse
		64: logicFalseElse
		65: print
		66: tagTimer
		67: setNoRemove
		68: resetNoRemove
	*/
	auto writeCommand = [&file, &commands](std::string scriptName, const char* scriptFormat = nullptr)
		{
			scriptName += "(";
			while (scriptFormat && *scriptFormat != NULL)
			{
				switch (*scriptFormat)
				{
				case 'I':
					scriptName += std::to_string(file.Read<unsigned int>(0, true));
					break;
				case 'i':
					scriptName += std::to_string(file.Read<int>(0, true));
					break;
				case 'X':
				{
					addr_t addr = file.Read<addr_t>(0, true);
					if (*(scriptFormat + 1) == 'O')
					{
						if (addr == 0)
							scriptName += "@CameraTarget#";
						else
							scriptName += FindInstanceName(file, addr) + "#";
						scriptFormat++;
					}
					else
						scriptName += "0x";
					scriptName += Hexify(addr);
					break;
				}
				case 'x':
				{
					int i = file.Read<int>(0, true);
					if (i < 0)
					{
						scriptName += "-";
						i *= -1;
					}
					scriptName += "0x" + Hexify(i);

					break;
				}
				case 's':
				{
					scriptName += std::to_string(file.Read<short>(0, true));
					break;
				}
				}

				if (*++scriptFormat)
				{
					scriptName += ",";
				}
			}
			commands.push_back("  " + scriptName + ")");
		};
	EScriptType scriptId = (EScriptType)0;
	std::vector<addr_t> jumps;
	unsigned int i = 0;
	commands.push_back("[0x" + Hexify(file._getoffset()) + "]");
	for(; i <= sz; ++i)
	{
		scriptId = file.Read<EScriptType>(0, true);

		switch ((EScriptType)((size_t)scriptId & 0xFFFF))
		{
		case EScriptType::hideObject:
			writeCommand("hideObject", "XO");
			break;

		case EScriptType::unhideObject:
			writeCommand("unhideObject", "XO");
			break;

		case EScriptType::timer:
			writeCommand("timer", "I");
			break;

			// gotoFrame
			// changeModel
			// startAniTex
			// stopAniTex

		case EScriptType::startSpline:
			writeCommand("startSpline", "X");
			break;
			
			// stopSpline

		case EScriptType::deathZ:
			writeCommand("deathZ", "i");
			break;

		case EScriptType::dsignal:
			writeCommand("dsignal", "XX");
			break;

			// may be deprecated
		case EScriptType::gsignal:
			writeCommand("gsignal", "XXXX");
			break;

		case EScriptType::lightGroup:
			writeCommand("lightGroup", "I");
			break;

		case EScriptType::cameraAdjust:
			writeCommand("cameraAdjust", "I");
			break;

		case EScriptType::cameraMode:
			writeCommand("cameraMode", "I");
			break;

		case EScriptType::camera:
			writeCommand("camera", "X");
			break;

		case EScriptType::cameraTimer:
			writeCommand("cameraTimer", "I");
			break;

		case EScriptType::cameraSmooth:
			writeCommand("cameraSmooth", "I");
			break;

		case EScriptType::cameraValueQQ:
			writeCommand("cameraValue??", "IX");
			break;

		case EScriptType::cameraLock:
			writeCommand("cameraLock", "I");
			break;

		case EScriptType::cameraUnlock:
			writeCommand("cameraUnlock", "I");
			break;

		case EScriptType::cameraSave:
			writeCommand("cameraSave", "I");
			break;

		case EScriptType::cameraRestore:
			writeCommand("cameraRestore", "I");
			break;

			// temp hack
		case EScriptType::teleport:
		{
			//writeCommand("teleport", "XXXX");
			short x, y, z, ox, oy, oz, a;
			x = file.Read<i16>(0, true);
			z = file.Read<i16>(0, true);
			y = file.Read<i16>(0, true);
			a = file.Read<i16>(0, true);
			ox = file.Read<i16>(0, true);
			oz = file.Read<i16>(0, true);
			oy = file.Read<i16>(0, true);
			(void)file.Read<i16>(0, true);
			std::string cmd = "  teleport(";
			cmd += std::to_string(x);
			cmd += ",";
			cmd += std::to_string(y);
			cmd += ",";
			cmd += std::to_string(z);
			cmd += ",";
			cmd += std::to_string(a * 180.f / 2048.f);
			cmd += ",";
			cmd += std::to_string(ox);
			cmd += ",";
			cmd += std::to_string(oy);
			cmd += ",";
			cmd += std::to_string(oz);
			commands.push_back(cmd + ")");
			break;
		}

		case EScriptType::farPlane:
			writeCommand("farPlane", "I");
			break;

		case EScriptType::soundStartSequence:
			writeCommand("soundStartSequence", "II");
			break;

			// soundStopSlot
			// soundPauseSlot
			// soundResumeSlot
			// soundMuteChannel
			// soundUnmuteChannel

		case EScriptType::times:
			writeCommand("times", "I");
			break;

		case EScriptType::freeze:
			writeCommand("freeze", "XO");
			break;

		case EScriptType::unfreeze:
			writeCommand("unfreeze", "XO");
			break;

		case EScriptType::freezeAll:
			writeCommand("freezeAll");
			break;

		case EScriptType::unfreezeAll:
			writeCommand("unfreezeAll");
			break;

		case EScriptType::hideBG:
			writeCommand("hideBG");
			break;

		case EScriptType::unhideBG:
			writeCommand("unhideBG");
			break;

		case EScriptType::hideBGObject:
			writeCommand("hideBGObject", "X");
			break;

		case EScriptType::unhideBGObject:
			writeCommand("unhideBGObject", "X");
			break;

		case EScriptType::mirror:
			writeCommand("mirror");
			break;

		case EScriptType::unmirror:
			writeCommand("unmirror");
			break;

		case EScriptType::setMirror:
			writeCommand("setMirror", "X");
			break;

		case EScriptType::fogNear:
			writeCommand("fogNear", "I");
			break;

		case EScriptType::fogFar:
			writeCommand("fogFar", "I");
			break;

		case EScriptType::startVertexMorph:
			writeCommand("startVertexMorph", "X");
			break;

		case EScriptType::stopVertexMorph:
			writeCommand("stopVertexMorph", "X");
			break;

		case EScriptType::logicValue:
			writeCommand("logicValue", "II");
			break;

		case EScriptType::cameraShake:
			writeCommand("cameraShake", "II");
			break;

			// logicAnd
			// logicOr
			// logicXor

		case EScriptType::logicTrue:
			writeCommand("logicTrue", "II");
			break;

		case EScriptType::logicFalse:
			writeCommand("logicFalse", "II");
			break;

		case EScriptType::callSignal:
		{
			addr_t signal = file.Read<addr_t>(0);
			writeCommand("callSignal", "X");
			if (!level.signals.contains(signal))
			{
				ParseCommands(level, file, signal, level.signals[signal].commands);
				if (!level.signals[signal].commands.empty())
					level.signals[signal].commands[0] += "#callSignal";
			}

			break;
		}

		case EScriptType::offset:
			writeCommand("offset", "XI");
			break;

		case EScriptType::logicAdd:
			writeCommand("logicAdd", "III");
			break;

			// logicSub

		case EScriptType::_goto:
			jumps.push_back(file.Read<addr_t>(0));
			writeCommand("goto", "X");
			break;

		case EScriptType::label:
			commands.push_back("label: [0x" + Hexify(file._getoffset()) + "]");
			break;

		case EScriptType::end:
		{
			writeCommand("end");
			if (!jumps.empty())
			{
				file.seek(jumps.back() - 4, true);
				jumps.pop_back();
			}
			break;
		}

			// gosub

		case EScriptType::stopPlayerControl:
			writeCommand("stopPlayerControl");
			break;

		case EScriptType::startPlayerControl:
			writeCommand("startPlayerControl");
			break;

		case EScriptType::setPlayerControl:
			writeCommand("setPlayerControl", "II");
			break;

		case EScriptType::launch:
			writeCommand("launch", "XI");
			break;

		case EScriptType::costumeChange:
		{
			//writeCommand("costumeChange", "X");
			addr_t costumeNameAddress = file.Read<addr_t>(0, true);
			char costumeName[9]{ 0 };
			if (costumeNameAddress == 0)
				strncpy_s(costumeName, "gex", 3);
			else
			{
				strncpy_s(costumeName, file.ptrAt<char>(costumeNameAddress), 8);
			}
			std::string cmd = "  costumeChange(";
			cmd += costumeName;
			commands.push_back(cmd + ")");
			break;
		}

			// gameValue

		case EScriptType::setZSignal:
		{
			addr_t signal = file.Read<addr_t>(4);
			addr_t signal2 = file.Read<addr_t>(8);
			writeCommand("setZSignal", "ssXX");
			if (!level.signals.contains(signal))
			{
				ParseCommands(level, file, signal, level.signals[signal].commands);
				if (!level.signals[signal].commands.empty())
					level.signals[signal].commands[0] += "#setZSignal";
			}
			if (!level.signals.contains(signal2))
			{
				ParseCommands(level, file, signal2, level.signals[signal2].commands);
				if (!level.signals[signal2].commands.empty())
					level.signals[signal2].commands[0] += "#setZSignal";
			}
			break;

		}

		case EScriptType::resetZSignal:
			writeCommand("resetZSignal", "I");
			break;

		case EScriptType::SoundEffect:
			writeCommand("SoundEffect", "I");
			break;

			// MusicControl
			// LevelChange(0) *
			// VoiceControl
			// BankChange
			// GameLoadSave

		case EScriptType::HideObjectGroup:
			writeCommand("HideObjectGroup", "X");
			break;

		case EScriptType::UnhideObjectGroup:
			writeCommand("UnhideObjectGroup", "X");
			break;

			// Shards

		case EScriptType::cameraSplineQQ:
		{
			//writeCommand("cameraSpline??", "IX");
			std::string cmd = "  cameraSpline??(";
			int i = file.Read<int>(0, true);
			addr_t spline = file.Read<addr_t>(0, true);
			if (i >= 0 && i <= 1)
				cmd += (i == 0) ? ("CameraPosition") : ("CameraTarget");
			else
				cmd += std::to_string(i);
			cmd += ", 0x";
			cmd += Hexify(spline);
			commands.push_back(cmd + ")");
			break;
		}

		case EScriptType::screenWipe:
		{
			//writeCommand("screenWipe", "X");
			std::string cmd = "  screenWipe(0x";
			cmd += Hexify(file.Read<u16>(0, true));
			cmd += ", ";
			cmd += std::to_string(file.Read<i16>(0, true));
			commands.push_back(cmd + ")");
			break;
		}

			// VoiceQueue
			// VoiceUnqueue
			// VoiceRequest
			// VoiceClearQueue
			// VoiceForce

		case EScriptType::setMusicVariable:
			writeCommand("setMusicVariable", "II");
			break;

			// introActive

		case EScriptType::introFX:
			writeCommand("introFX", "iI");
			break;

		case EScriptType::gotoPos:
		{
			std::string cmd;
			cmd += "  gotoPos(";
			cmd += std::to_string(file.Read<short>(0));
			cmd += ", ";
			cmd += std::to_string(file.Read<short>(4));
			cmd += ", ";
			cmd += std::to_string(file.Read<short>(2));
			file.seek(file._getoffset() + 8, true);
			commands.push_back(cmd + ")");
			break;
		}

			// frame

		case EScriptType::birthObject:
			writeCommand("birthObject", "XO");
			break;

		case EScriptType::blendStart:
			writeCommand("blendStart", "I");
			break;

		case EScriptType::miscValue:
			writeCommand("miscValue", "IX");
			break;

			// VoiceDisable
			// VoiceReEnable

		// A loop maybe?
		case EScriptType::setTimes:
		{
			writeCommand("setTimes", "XI"); // address : x
			break;

		}

		case EScriptType::screenWipeColor:
		{
			std::string cmd = "  screenWipeColor(";
			cmd += std::to_string(file.Read<byte>(2));
			cmd += ", ";
			cmd += std::to_string(file.Read<byte>(0));
			cmd += ", ";
			cmd += std::to_string(file.Read<byte>(1));
			file.seek(file._getoffset() + 4, true);
			commands.push_back(cmd + ")");
			break;
		}

		case EScriptType::relocate:
			writeCommand("relocate", "XXXX");
			break;

		case EScriptType::logicTrueElse:
			jumps.push_back(file.Read<addr_t>(4));
			writeCommand("logicTrueElse", "IX");
			break;

		case EScriptType::logicFalseElse:
			jumps.push_back(file.Read<addr_t>(4));
			writeCommand("logicFalseElse", "IX");
			break;

			// print

		case EScriptType::tagTimer:
			writeCommand("tagTimer");
			break;

			// setNoRemove

		case EScriptType::setNoRemove:
			writeCommand("setNoRemove", "XO");
			break;

		case EScriptType::resetNoRemove:
			writeCommand("resetNoRemove", "XO");
			break;

		default:
			if (((size_t)(scriptId) & 0xFFFF) < scripts.size())
				commands.push_back(scripts[((size_t)scriptId) & 0xFFFF]);
			else
				commands.push_back("Unknown ID. Maybe this is a parameter? (" + Hexify((size_t)scriptId) + ")\n");
		}
	}

	if (scriptId != EScriptType::end)
	{
		printf("%x: Mismatch! Expected %d scripts, but did not reach an end?\n", address, sz + 1);
		for (auto& c : commands)
			printf("\t%s\n", c.c_str());
	}
	else if (i != (sz + 1))
	{
		printf("%x: Mismatch! Expected %d scripts but got %d?\n", address, sz + 1, i);
		for (auto& c : commands)
			printf("\t%s\n", c.c_str());
	}

	file.pop();
}