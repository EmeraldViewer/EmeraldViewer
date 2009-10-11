#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "emeraldboobutils.h"

namespace
{

	class BoobTests : public ::testing::Test
	{
	public:
		void SetUp()
		{
			m_config.enabled = true;
		}

		EmeraldGlobalBoobConfig m_config;
		EmeraldAvatarLocalBoobConfig m_localConfig;
		EmeraldBoobState m_oldState;
		EmeraldBoobInputs m_inputs;
	};

	TEST_F(BoobTests, BoobCodeReturnsActualBoobGravAsBoobGravWhenSettingIsOff)
	{
		m_config.enabled = false;
		m_localConfig.actualBoobGrav = 1.0f;
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		SCOPED_TRACE(m_oldState);
		SCOPED_TRACE(m_inputs);
		EmeraldBoobState newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
		SCOPED_TRACE(newState);
		EXPECT_EQ(m_localConfig.actualBoobGrav, newState.boobGrav);
	}

	TEST_F(BoobTests, BoobCodeReturnsActualBoobGravAsBoobGravInEditAppearanceMode)
	{
		m_inputs.appearanceFlag = true;
		m_localConfig.actualBoobGrav = 1.0f;
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		SCOPED_TRACE(m_oldState);
		SCOPED_TRACE(m_inputs);
		EmeraldBoobState newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
		SCOPED_TRACE(newState);
		EXPECT_EQ(m_localConfig.actualBoobGrav, newState.boobGrav);
	}

	TEST_F(BoobTests, BoobCodeStoresElapsedTime)
	{
		m_oldState.elapsedTime = 123.0f;
		m_inputs.elapsedTime = 234.0f;
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		SCOPED_TRACE(m_oldState);
		SCOPED_TRACE(m_inputs);
		EmeraldBoobState newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
		SCOPED_TRACE(newState);
		EXPECT_EQ(m_inputs.elapsedTime, newState.elapsedTime);
	}

	TEST_F(BoobTests, BoobCodeStoresChestPosition)
	{
		m_oldState.chestPosition = LLVector3(1.0f, 2.0f, 3.0f);
		m_inputs.chestPosition = LLVector3(2.0f, 3.0f, 4.0f);
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		SCOPED_TRACE(m_oldState);
		SCOPED_TRACE(m_inputs);
		EmeraldBoobState newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
		SCOPED_TRACE(newState);
		EXPECT_EQ(m_inputs.chestPosition, newState.chestPosition);
	}

	TEST_F(BoobTests, BoobCodeStoresChestRotation)
	{
		m_oldState.chestRotation = LLQuaternion(1.0f, 2.0f, 3.0f, 4.0f);
		m_inputs.chestRotation = LLQuaternion(2.0f, 3.0f, 4.0f, 5.0f);
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		SCOPED_TRACE(m_oldState);
		SCOPED_TRACE(m_inputs);
		EmeraldBoobState newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
		SCOPED_TRACE(newState);
		EXPECT_EQ(m_inputs.chestRotation, newState.chestRotation);
	}

	TEST_F(BoobTests, BoobCodeCorrectlyCalculatesFrameDuration)
	{
		m_oldState.elapsedTime = 123.0f;
		m_inputs.elapsedTime = 234.0f;
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		SCOPED_TRACE(m_oldState);
		SCOPED_TRACE(m_inputs);
		EmeraldBoobState newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
		SCOPED_TRACE(newState);
		EXPECT_EQ(m_inputs.elapsedTime - m_oldState.elapsedTime, newState.frameDuration);
	}

	TEST_F(BoobTests, BoobCodeCorrectlyCalculatesChestDisplacement)
	{
		m_oldState.chestPosition = LLVector3(1.0f, 2.0f, 3.0f);
		m_inputs.chestPosition = LLVector3(2.0f, 3.0f, 4.0f);
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		SCOPED_TRACE(m_oldState);
		SCOPED_TRACE(m_inputs);
		EmeraldBoobState newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
		SCOPED_TRACE(newState);
		EXPECT_EQ(m_inputs.chestPosition - m_oldState.chestPosition, newState.chestDisplacement);
	}

	TEST_F(BoobTests, BoobCodeCorrectlyCalculatesLocalChestDisplacement)
	{
		m_oldState.chestPosition = LLVector3(1.0f, 2.0f, 3.0f);
		m_inputs.chestPosition = LLVector3(2.0f, 3.0f, 4.0f);
		m_inputs.chestRotation = LLQuaternion(3.0f, 4.0f, 5.0f, 6.0f);
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		SCOPED_TRACE(m_oldState);
		SCOPED_TRACE(m_inputs);
		EmeraldBoobState newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
		SCOPED_TRACE(newState);
		EXPECT_EQ((m_inputs.chestPosition - m_oldState.chestPosition) * ~m_inputs.chestRotation, newState.localChestDisplacement);
	}

	TEST_F(BoobTests, BoobCodeClampsBoobGravFromDisplacementEffectAtMinusOnePointFive)
	{
		m_localConfig.actualBoobGrav = 1.0f;
		m_oldState.chestPosition = LLVector3(0.0f, 0.0f, 0.0f);
		m_oldState.boobGrav = m_localConfig.actualBoobGrav;
		m_inputs.chestPosition = LLVector3(0.0f, 0.0f, -100.0f);
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		SCOPED_TRACE(m_oldState);
		SCOPED_TRACE(m_inputs);
		EmeraldBoobState newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
		SCOPED_TRACE(newState);
		EXPECT_LE(-1.5f, newState.boobGrav);
	}

	TEST_F(BoobTests, BoobCodeClampsBoobGravFromDisplacementEffectAtPlusTwo)
	{
		m_localConfig.actualBoobGrav = 1.0f;
		m_oldState.chestPosition = LLVector3(0.0f, 0.0f, 0.0f);
		m_oldState.boobGrav = m_localConfig.actualBoobGrav;
		m_inputs.chestPosition = LLVector3(0.0f, 0.0f, 100.0f);
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		SCOPED_TRACE(m_oldState);
		SCOPED_TRACE(m_inputs);
		EmeraldBoobState newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
		SCOPED_TRACE(newState);
		EXPECT_GE(2.0f, newState.boobGrav);
	}

	TEST_F(BoobTests, BoobCodeReturnsBoobsToActualBoobGravWithinFiveSecondsAtTenFPS)
	{
		m_localConfig.actualBoobGrav = 1.0f;
		EmeraldBoobBounceState bounceState;
		bounceState.bounceStartAmplitude = 1.0;
		bounceState.bounceStartFrameDuration = 0.1f;
		m_oldState.bounceStates.push_front(bounceState);
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		EmeraldBoobState newState;
		for(F32 t=0;t<5.0f;t+=0.1f)
		{
			m_inputs.elapsedTime=t;
			SCOPED_TRACE(m_oldState);
			SCOPED_TRACE(m_inputs);
			newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
			SCOPED_TRACE(newState);
			//printf("After %f seconds, boobGrav: %f\n", t, newState.boobGrav);
			m_oldState = newState;
		}
		SCOPED_TRACE(newState);
		EXPECT_EQ(m_localConfig.actualBoobGrav, newState.boobGrav);
	}

	TEST_F(BoobTests, BoobCodeReducesAmplitudeSlowlyEnoughThatThereIsStillMovementAfterOneSecondAtTenFPS)
	{
		m_localConfig.actualBoobGrav = 1.0f;
		EmeraldBoobBounceState bounceState;
		bounceState.bounceStartAmplitude = 1.0;
		bounceState.bounceStartFrameDuration = 0.1f;
		m_oldState.bounceStates.push_front(bounceState);
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		EmeraldBoobState newState;
		for(F32 t=0;t<1.0f;t+=0.1f)
		{
			m_inputs.elapsedTime=t;
			SCOPED_TRACE(m_oldState);
			SCOPED_TRACE(m_inputs);
			newState = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, m_oldState, m_inputs);
			//printf("After %f seconds, boobGrav: %f\n", t, newState.boobGrav);
			m_oldState = newState;
		}
		SCOPED_TRACE(newState);
		EXPECT_LT(0.0f, fabs(newState.boobGrav-m_localConfig.actualBoobGrav));
	}

	TEST_F(BoobTests, BoobCodeBouncePartBehavesTheSameAtFiveTenAndFiftyFPS)
	{
		m_localConfig.actualBoobGrav = 1.0f;
		EmeraldBoobBounceState bounceState;
		bounceState.bounceStartAmplitude = 1.0;
		bounceState.bounceStartFrameDuration = 0.1f;
		m_oldState.bounceStates.push_front(bounceState);
		EmeraldBoobState oldState05FPS;
		oldState05FPS.bounceStates.push_front(bounceState);
		EmeraldBoobState oldState10FPS;
		oldState10FPS.bounceStates.push_front(bounceState);
		EmeraldBoobState oldState50FPS;
		oldState50FPS.bounceStates.push_front(bounceState);
		SCOPED_TRACE(m_config);
		SCOPED_TRACE(m_localConfig);
		EmeraldBoobState newState05FPS;
		EmeraldBoobState newState10FPS;
		EmeraldBoobState newState50FPS;
		for(int i=0;i<500;i++)
		{
			F32 t=0.02f*i;
			m_inputs.elapsedTime=t;
			newState50FPS = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, oldState50FPS, m_inputs);
			//printf("Frame %d, 50 FPS, boobGrav: %f\n",i,newState50FPS.boobGrav);
			if(i%5==0) {
				newState10FPS = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, oldState10FPS, m_inputs);
				//printf("Frame %d, 10 FPS, boobGrav: %f\n",i/5,newState50FPS.boobGrav);
				EXPECT_FLOAT_EQ(newState50FPS.boobGrav, newState10FPS.boobGrav);
			}
			if(i%10==0) {
				newState05FPS = EmeraldBoobUtils::idleUpdate(m_config, m_localConfig, oldState05FPS, m_inputs);
				//printf("Frame %d,  5 FPS, boobGrav: %f\n",i/10,newState50FPS.boobGrav);
				EXPECT_FLOAT_EQ(newState50FPS.boobGrav, newState05FPS.boobGrav);
			}
		}
	}

	TEST(AbsTest, AbsWorksOnF32AsExpected)
	{
		EXPECT_EQ((F32)1.0f, fabs((F32)1.0f));
		EXPECT_EQ((F32)1.0f, fabs((F32)-1.0f));
	}

};

