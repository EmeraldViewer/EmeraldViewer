#include "emeraldboobutils.h"

std::ostream &operator<<(std::ostream &os, const EmeraldGlobalBoobConfig &v)
{
	os << "EmeraldBoobConfig" << std::endl;
	os << "enabled: " << v.enabled << std::endl;
	os << "mass: " << v.mass << std::endl;
	os << "hardness: " << v.hardness << std::endl;
	os << "zMax: " << v.zMax << std::endl;
	os << "velMin: " << v.velMin << std::endl;
	os << "velMax: " << v.velMax << std::endl;
	os << "zInfluence: " << v.zInfluence << std::endl;
	os << "friction: " << v.friction << std::endl;
	os << "frictionFraction: " << v.frictionFraction << std::endl;
	return os;
}

std::ostream &operator<<(std::ostream &os, const EmeraldAvatarLocalBoobConfig &v)
{
	os << "EmeraldAvatarLocalBoobConfig" << std::endl;
	os << "actualBoobGrav: " << v.actualBoobGrav << std::endl;
	os << "boobSize: " << v.boobSize << std::endl;
	return os;
}

std::ostream &operator<<(std::ostream &os, const EmeraldBoobState &v)
{
	os << "EmeraldBoobState" << std::endl;
	os << "boobGrav: " << v.boobGrav << std::endl;
	os << "chestPosition: " << v.chestPosition << std::endl;
	os << "chestRotation: " << v.chestRotation << std::endl;
	os << "elapsedTime: " << v.elapsedTime << std::endl;
	os << "frameDuration: " << v.frameDuration << std::endl;
	os << "chestDisplacement: " << v.chestDisplacement << std::endl;
	os << "localChestDisplacement: " << v.localChestDisplacement << std::endl;
	os << "displacementForce: " << v.displacementForce << std::endl;
	os << "mysteryValue: " << v.mysteryValue << std::endl;
	os << "Number of bounceStates: " << v.bounceStates.size() << std::endl;
	return os;
}

std::ostream &operator<<(std::ostream &os, const EmeraldBoobInputs &v)
{
	os << "EmeraldBoobInputs" << std::endl;
	os << "chestPosition: " << v.chestPosition << std::endl;
	os << "chestRotation: " << v.chestRotation << std::endl;
	os << "elapsedTime: " << v.elapsedTime << std::endl;
	os << "appearanceFlag: " << v.appearanceFlag << std::endl;
	os << "appearanceAnimating: " << v.appearanceAnimating << std::endl;
	return os;
}

std::ostream &operator<<(std::ostream &os, const EmeraldBoobBounceState &v)
{
	os << "EmeraldBoobBounceState" << std::endl;
	os << "bounceStart: " << v.bounceStart << std::endl;
	os << "bounceStartAmplitude: " << v.bounceStartAmplitude << std::endl;
	os << "bounceStartFrameDuration: " << v.bounceStartFrameDuration << std::endl;
	return os;
}

EmeraldBoobState EmeraldBoobUtils::idleUpdate(const EmeraldGlobalBoobConfig &config, const EmeraldAvatarLocalBoobConfig &localConfig, const EmeraldBoobState &oldState, const EmeraldBoobInputs &inputs)
{
	EmeraldBoobState newState;
	newState.boobGrav = localConfig.actualBoobGrav;
	if(!config.enabled || inputs.appearanceFlag || inputs.appearanceAnimating)
		return newState;

	//F32 avatarLocalMass = config.mass * localConfig.boobSize;

	newState.elapsedTime = inputs.elapsedTime;
	newState.frameDuration = inputs.elapsedTime - oldState.elapsedTime;
	newState.chestPosition = inputs.chestPosition;
	newState.chestRotation = inputs.chestRotation;
	newState.chestDisplacement = inputs.chestPosition - oldState.chestPosition;
	newState.localChestDisplacement = newState.chestDisplacement * ~inputs.chestRotation;


	std::list<EmeraldBoobBounceState> bounceStates = oldState.bounceStates;

	if(fabs(newState.localChestDisplacement.length())>0.0f) {
		LLVector3 displacementInfluence = newState.localChestDisplacement;
		displacementInfluence *= LLVector3(0.3f, 0.3f, 1.0f);
		F32 clampedDisplacementInfluenceLength = llclamp(displacementInfluence.length(), 0.0f, 1.0f);
		if(displacementInfluence[VZ]<0.f)
			clampedDisplacementInfluenceLength= -clampedDisplacementInfluenceLength;
		EmeraldBoobBounceState bounceState;
		bounceState.bounceStart = inputs.elapsedTime;
		bounceState.bounceStartFrameDuration = newState.frameDuration;
		bounceState.bounceStartAmplitude = clampedDisplacementInfluenceLength*200.f;
		bounceStates.push_front(bounceState);
	}

	F32 totalNewAmplitude = 0.0f;
	//std::cout << "Beginning bounce State processing at time " << inputs.elapsedTime << std::endl;
	while(!bounceStates.empty()) {
		EmeraldBoobBounceState bounceState = bounceStates.front();
		//std::cout << "Now processing " << bounceState;
		bounceStates.pop_front();
		F32 bounceTime = newState.elapsedTime-bounceState.bounceStart;
		F32 newAmplitude = bounceState.bounceStartAmplitude*pow(2.0f, -bounceTime)*cos(10.f*inputs.elapsedTime);
		if(fabs(newAmplitude) < 0.01f) {
			newAmplitude = 0.0f;
		} else {
			newState.bounceStates.push_front(bounceState);
		}
		totalNewAmplitude+=(newAmplitude*bounceState.bounceStartFrameDuration);
	}
	//std::cout << "Total new amplitude: " << totalNewAmplitude << std::endl;
	newState.boobGrav = localConfig.actualBoobGrav + totalNewAmplitude;

	// newState.boobGrav = oldState.boobGrav;
	// newState.mysteryValue = oldState.mysteryValue;

	// F32 boobVel = 0.f;
	// boobVel = newState.localChestDisplacement.mV[VZ];
	// boobVel += newState.localChestDisplacement[VX] * 0.3f;
	// boobVel += newState.localChestDisplacement.mV[VY] * 0.3f;
	// boobVel = llclamp(boobVel, -config.velMax, config.velMax);

	// //if movement's negligible, just return. Prevents super slight jiggling.
	// if(fabs(boobVel) <= config.velMin)
	// 	boobVel = 0.0f;

	// boobVel *= config.zInfluence * (llclamp(localConfig.boobSize, 0.0f, 0.5f) / 0.5f);


	// F32 FPS = llclamp(1.f/newState.frameDuration, 1.f, 50.f);

	// F32 boobHardness = -(1.5f - ( (1.5f - (config.hardness))*((FPS - 1.f )/ (50.f - 1.f)) ));
	// F32 friction = (1.f - (1.f - config.friction)) + ((1.f - config.friction) - (1.f - (1.f - config.friction)))*((FPS - 1.f )/ (50.f - 1.f));

	// newState.boobGrav += boobVel * config.mass;
	// newState.mysteryValue += boobHardness * (newState.boobGrav-oldState.boobGrav);
	// newState.mysteryValue *= friction;

	// newState.boobGrav += newState.mysteryValue;

	newState.boobGrav = llclamp(newState.boobGrav, -1.5f, 2.0f);

	return newState;
}
