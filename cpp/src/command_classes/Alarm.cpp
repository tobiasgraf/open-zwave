//-----------------------------------------------------------------------------
//
//	Alarm.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_ALARM
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "command_classes/CommandClasses.h"
#include "command_classes/Alarm.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"
#include <map>

#include "value_classes/ValueByte.h"

using namespace OpenZWave;

enum AlarmCmd
{
	AlarmCmd_Get			= 0x04,
	AlarmCmd_Report			= 0x05,
	// Version 2
	AlarmCmd_SupportedGet		= 0x07,
	AlarmCmd_SupportedReport	= 0x08,
	// Version 3
	AlarmCmd_SupportedEventGet      = 0x01,
	AlarmCmd_SupportedEventReport   = 0x02
};

enum
{
	AlarmIndex_Type = 0,
	AlarmIndex_Level,
	AlarmIndex_SourceNodeId
};

enum
{
	Alarm_General = 0,
	Alarm_Smoke,
	Alarm_CarbonMonoxide,
	Alarm_CarbonDioxide,
	Alarm_Heat,
	Alarm_Flood,
	Alarm_Access_Control,
	Alarm_Burglar,
	Alarm_Power_Management,
	Alarm_System,
	Alarm_Emergency,
	Alarm_Clock,
	Alarm_Appliance,
	Alarm_HomeHealth,
	Alarm_Count
};

static char const* c_alarmTypeName[] =
{
		"General",
		"Smoke",
		"Carbon Monoxide",
		"Carbon Dioxide",
		"Heat",
		"Flood",
		"Access Control",
		"Burglar",
		"Power Management",
		"System",
		"Emergency",
		"Clock",
		"Appliance",
		"HomeHealth"
};

//for Version 3 where each alarm type or Notification Type can have more than one event
enum
{
    Alarm_Smoke_EventInactive                   = 0x00,
    Alarm_Smoke_SmokeDetected                   = 0x01,
    Alarm_Smoke_SmokeDetectedUnknownLocation    = 0x02,
    Alarm_Smoke_SmokeAlarmTest                  = 0x03,
    Alarm_Smoke_ReplacementRequired             = 0x04,
    Alarm_Smoke_Count                                 ,
    Alarm_Smoke_UnknownEvent                    = 0xFE
};

enum
{
    Alarm_CarbonMonoxide_EventInactive                  = 0x00,
    Alarm_CarbonMonoxide_CODetected                     = 0x01,
    Alarm_CarbonMonoxide_CODetectedUnknownLocation      = 0x02,
    Alarm_CarbonMonoxide_COTest                         = 0x03,
    Alarm_CarbonMonoxide_ReplacementRequired            = 0x04,
    Alarm_CarbonMonoxide_Count,
    Alarm_CarbonMonoxide_UnknownEvent                   = 0xFE
};

enum
{
    Alarm_CarbonDioxide_EventInactive               = 0x00,
    Alarm_CarbonDioxide_CO2Detected                 = 0x01,
    Alarm_CarbonDioxide_CO2DetectedUnknownLocation  = 0x02,
    Alarm_CarbonDioxide_CO2Test                     = 0x03,
    Alarm_CarbonDioxide_ReplacementRequired         = 0x04,
    Alarm_CarbonDioxide_Count,
    Alarm_CarbonDioxide_UnknownEvent                = 0xFE
};

enum
{
    Alarm_Heat_EventInactive                        = 0x00,
    Alarm_Heat_OverheatDetected                     = 0x01,
    Alarm_Heat_OverheatDetectedUnknownLocation      = 0x02,
    Alarm_Heat_RapidTemperatureRise                 = 0x03,
    Alarm_Heat_RapidTemperatureRiseUnknownLocation  = 0x04,
    Alarm_Heat_UnderHeatDetected                    = 0x05,
    Alarm_Heat_UnderHeatDetectedUnknownLocation     = 0x06,
    Alarm_Heat_Count,
    Alarm_Heat_UnknownEvent                         = 0x0FE
};

enum
{
    Alarm_Flood_EventInactive                       = 0x00,
    Alarm_Flood_WaterLeakDetected                   = 0x01,
    Alarm_Flood_WaterLeakDetectedUnknownLocation    = 0x02,
    Alarm_Flood_WaterLevelDropped                   = 0x03,
    Alarm_Flood_WaterLevelDroppedUnknownLocation    = 0x04,
    Alarm_Flood_ReplaceWaterFilter                  = 0x05,
    Alarm_Flood_Count,
    Alarm_Flood_UnknownEvent                        = 0xFE
};

enum
{
    Alarm_Access_Control_EventInactive                                                      = 0x00,
    Alarm_Access_Control_ManualLockOperation                                                = 0x01,
    Alarm_Access_Control_ManualUnlockOperation                                              = 0x02,
    Alarm_Access_Control_RFLockOperation                                                    = 0x03,
    Alarm_Access_Control_RFUnlockOperation                                                  = 0x04,
    Alarm_Access_Control_KeypadLockOperation                                                = 0x05,
    Alarm_Access_Control_KeypadUnlockOperation                                              = 0x06,
    Alarm_Access_Control_ManualNotFullyLockedOperation                                      = 0x07,
    Alarm_Access_Control_RFNotFullyLockedOperation                                          = 0x08,
    Alarm_Access_Control_AutoLockLockedOperation                                            = 0x09,
    Alarm_Access_Control_AutoLockNotFullyOperation                                          = 0x0A,
    Alarm_Access_Control_LockJammed                                                         = 0x0B,
    Alarm_Access_Control_AllUserCodesDeleted                                                = 0x0C,
    Alarm_Access_Control_SingleUserCodeDeleted                                              = 0x0D,
    Alarm_Access_Control_NewUserCodeAdded                                                   = 0x0E,
    Alarm_Access_Control_NewUserCodeNotAddedDueToDuplicateCode                              = 0x0F,
    Alarm_Access_Control_KeypadTemporaryDisabled                                            = 0x10,
    Alarm_Access_Control_KeypadBusy                                                         = 0x11,
    Alarm_Access_Control_NewProgramCodeEnteredUniqueCodeForLockConfiguraton                 = 0x12,
    Alarm_Access_Control_ManuallyEnterUserAccessCodeExceedsCodeLimit                        = 0x13,
    Alarm_Access_Control_UnlockByRFWithInvalidUserCode                                      = 0x14,
    Alarm_Access_Control_LockedByRFWithInvalidUserCodes                                     = 0x15,
    Alarm_Access_Control_WindowDoorIsOpen                                                   = 0x16,
    Alarm_Access_Control_WindowDoorIsClosed                                                 = 0x17,
    Alarm_Access_Control_BarrierPerformingInitializationProcess                             = 0x40,
    Alarm_Access_Control_BarrierOperationForceHasBeenExceeded                               = 0x41,
    Alarm_Access_Control_BarrierMotorHasExceededManufacturersOperationalTimeLimit           = 0x42,
    Alarm_Access_Control_BarrierOperationHasExceededPhysicalMechanicalLimits                = 0x43,
    Alarm_Access_Control_BarrierUnableToPerformRequestedOperationDueToUlRequirements        = 0x44,
    Alarm_Access_Control_BarrierUnattendedOperationHasBeenDisabledPerUlRequirements         = 0x45,
    Alarm_Access_Control_BarrierFailedToPerformRequestedOperationDeviceMalfunction          = 0x46,
    Alarm_Access_Control_BarrierVacationMode                                                = 0x47,
    Alarm_Access_Control_BarrierSafetyBeamObstacle                                          = 0x48,
    Alarm_Access_Control_BarrierSensorNotDetectedSupervisoryError                           = 0x49,
    Alarm_Access_Control_BarrierSensorLowBatteryWarning                                     = 0x4A,
    Alarm_Access_Control_BarrierDetectedShortInWallStationWires                             = 0x4B,
    Alarm_Access_Control_BarrierAssociatedWithNonZWaveRemoteControl                         = 0x4C,
    Alarm_Access_Control_Count,
    Alarm_Access_Control_UnknownEvent                                                       = 0xFE
};

enum
{
    Alarm_Burglar_EventInactive                         = 0x00,
    Alarm_Burglar_Intrusion                             = 0x01,
    Alarm_Burglar_IntrusionUnknownLocation              = 0x02,
    Alarm_Burglar_TamperingProductCoverRemoved          = 0x03,
    Alarm_Burglar_TamperingInvalidCode                  = 0x04,
    Alarm_Burglar_GlassBreakage                         = 0x05,
    Alarm_Burglar_GlassBreakageUnknownLocation          = 0x06,
    Alarm_Burglar_MotionDetection                       = 0x07,
    Alarm_Burglar_MotionDetectionUnknownLocation        = 0x08,
    Alarm_Burglar_Count,
    Alarm_Burglar_UnknownEvent                          = 0xFE
};

enum
{

    Alarm_Power_Management_EventInactive        = 0x00,
    Alarm_Power_Management_PowerApplied         = 0x01,
    Alarm_Power_Management_ACMainsDisconnected  = 0x02,
    Alarm_Power_Management_ACMainsReconnected   = 0x03,
    Alarm_Power_Management_SurgeDetected        = 0x04,
    Alarm_Power_Management_VoltageDropOrDrift   = 0x05,
    Alarm_Power_Management_OvercurrentDetected  = 0x06,
    Alarm_Power_Management_OvervoltageDetected  = 0x07,
    Alarm_Power_Management_OverloadDetected     = 0x08,
    Alarm_Power_Management_LoadError            = 0x09,
    Alarm_Power_Management_ReplaceBatterySoon   = 0x0A,
    Alarm_Power_Management_ReplaceBatteryNow    = 0x0B,
    Alarm_Power_Management_BatteryIsCharging    = 0x0C,
    Alarm_Power_Management_BatteryIsFullyCharged= 0x0D,
    Alarm_Power_Management_ChargeBatterySoon    = 0x0E,
    Alarm_Power_Management_ChargeBatteryNow     = 0x0F,
    Alarm_Power_Management_Count,
    Alarm_Power_Management_UnknownEvent         = 0xFE
};

enum
{
    Alarm_System_EventInactive                                              = 0x00,
    Alarm_System_SystemHardwareFailure                                      = 0x01,
    Alarm_System_SystemSoftwareFailure                                      = 0x02,
    Alarm_System_SystemHardwareFailureWithManufacturerProprietaryFaiureCode = 0x03,
    Alarm_System_SystemSoftwareFailureWithManufacturerProprietaryFaiureCode = 0x04,
    Alarm_System_Count,
    Alarm_System_UnknownEvent                                               = 0xFE
};

enum
{
    Alarm_Emergency_EventInactive           = 0x00,
    Alarm_Emergency_ContactPolice           = 0x01,
    Alarm_Emergency_ContactFireService      = 0x02,
    Alarm_Emergency_ContactMedicalService   = 0x03,
    Alarm_Emergency_Count,
    Alarm_Emergency_UnknownEvent            = 0xFE
};

enum
{
    Alarm_Clock_EventInactive       = 0x00,
    Alarm_Clock_WakeUpAlert         = 0x01,
    Alarm_Clock_TimerEnded          = 0x02,
    Alarm_Clock_TimeRemaining       = 0x03,
    Alarm_Clock_Count,
    Alarm_Clock_UnknownEvent        = 0xFE
};

enum
{
    Alarm_Appliance_EventInactive                   = 0x00,
    Alarm_Appliance_ProgramStarted                  = 0x01,
    Alarm_Appliance_ProgramInProgress               = 0x02,
    Alarm_Appliance_ProgramCompleted                = 0x03,
    Alarm_Appliance_PreplaceMainFilter              = 0x04,
    Alarm_Appliance_FailureToSetTargetTemperature   = 0x05,
    Alarm_Appliance_SupplyingWater                  = 0x06,
    Alarm_Appliance_WaterSupplyFailure              = 0x07,
    Alarm_Appliance_Boiling                         = 0x08,
    Alarm_Appliance_BoilingFailure                  = 0x09,
    Alarm_Appliance_Washing                         = 0x0A,
    Alarm_Appliance_WashingFailure                  = 0x0B,
    Alarm_Appliance_Rinsing                         = 0x0C,
    Alarm_Appliance_RinsingFailure                  = 0x0D,
    Alarm_Appliance_Draining                        = 0x0E,
    Alarm_Appliance_DrainingFailure                 = 0x0F,
    Alarm_Appliance_Spinning                        = 0x10,
    Alarm_Appliance_SpinningFailure                 = 0x11,
    Alarm_Appliance_Drying                          = 0x12,
    Alarm_Appliance_DryingFailure                   = 0x13,
    Alarm_Appliance_FanFailure                      = 0x14,
    Alarm_Appliance_CompressorFailure               = 0x15,
    Alarm_Appliance_Count,
    Alarm_Appliance_UnknownEvent                    = 0xFE
};

enum
{
    Alarm_HomeHealth_EventInactive                  = 0x00,
    Alarm_HomeHealth_LeavingBed                     = 0x01,
    Alarm_HomeHealth_SittingOnBed                   = 0x02,
    Alarm_HomeHealth_LyingOnBed                     = 0x03,
    Alarm_HomeHealth_PostureChanged                 = 0x04,
    Alarm_HomeHealth_SittingOnEdgeOfBed             = 0x05,
    Alarm_HomeHealth_VolatileOrganicCompoundLevel   = 0x06,
    Alarm_HomeHealth_Count,
    Alarm_HomeHealth_UnknownEvent                   = 0xFE
};

static int alarmPrefixCount[] =
{
        0, //
        0, //for Smoke Alarm
        Alarm_Smoke_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count + Alarm_Heat_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count + Alarm_Heat_Count + Alarm_Flood_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count + Alarm_Heat_Count + Alarm_Flood_Count + Alarm_Access_Control_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count + Alarm_Heat_Count + Alarm_Flood_Count + Alarm_Access_Control_Count + Alarm_Burglar_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count + Alarm_Heat_Count + Alarm_Flood_Count + Alarm_Access_Control_Count + Alarm_Burglar_Count + Alarm_Power_Management_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count + Alarm_Heat_Count + Alarm_Flood_Count + Alarm_Access_Control_Count + Alarm_Burglar_Count + Alarm_Power_Management_Count + Alarm_System_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count + Alarm_Heat_Count + Alarm_Flood_Count + Alarm_Access_Control_Count + Alarm_Burglar_Count + Alarm_Power_Management_Count + Alarm_System_Count + Alarm_Emergency_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count + Alarm_Heat_Count + Alarm_Flood_Count + Alarm_Access_Control_Count + Alarm_Burglar_Count + Alarm_Power_Management_Count + Alarm_System_Count + Alarm_Emergency_Count + Alarm_Clock_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count + Alarm_Heat_Count + Alarm_Flood_Count + Alarm_Access_Control_Count + Alarm_Burglar_Count + Alarm_Power_Management_Count + Alarm_System_Count + Alarm_Emergency_Count + Alarm_Clock_Count + Alarm_Appliance_Count,
        Alarm_Smoke_Count + Alarm_CarbonMonoxide_Count + Alarm_CarbonDioxide_Count + Alarm_Heat_Count + Alarm_Flood_Count + Alarm_Access_Control_Count + Alarm_Burglar_Count + Alarm_Power_Management_Count + Alarm_System_Count + Alarm_Emergency_Count + Alarm_Clock_Count + Alarm_Appliance_Count + Alarm_HomeHealth_Count,
};

static int alarmCounts[] =
{
        0, //for general Alarm
        Alarm_Smoke_Count,
        Alarm_CarbonMonoxide_Count,
        Alarm_CarbonDioxide_Count,
        Alarm_Heat_Count,
        Alarm_Flood_Count,
        Alarm_Access_Control_Count,
        Alarm_Burglar_Count,
        Alarm_Power_Management_Count,
        Alarm_System_Count,
        Alarm_Emergency_Count,
        Alarm_Clock_Count,
        Alarm_Appliance_Count,
        Alarm_HomeHealth_Count,
};

static std::map<uint8,std::map<uint8,char const*>> m_EventTypeName
{
    { Alarm_Smoke,
        {
                {Alarm_Smoke_EventInactive,                "Smoke Alarm Inactive"      },
                {Alarm_Smoke_SmokeDetected,                "Smoke"                     },
                {Alarm_Smoke_SmokeDetectedUnknownLocation, "Smoke"                     },
                {Alarm_Smoke_SmokeAlarmTest,               "Smoke Test"                },
                {Alarm_Smoke_ReplacementRequired,          "Smoke Replacement Required"},
                {Alarm_Smoke_UnknownEvent,                 "Unknown"                   },
        }
    },
    {Alarm_CarbonMonoxide,
        {
                {Alarm_CarbonMonoxide_EventInactive,             "Carbon Monoxide Alarm Inactive"      },
                {Alarm_CarbonMonoxide_CODetected,                "Carbon Monoxide"                     },
                {Alarm_CarbonMonoxide_CODetectedUnknownLocation, "Carbon Monoxide"                     },
                {Alarm_CarbonMonoxide_COTest,                    "Carbon Monoxide Test"                },
                {Alarm_CarbonMonoxide_ReplacementRequired,       "Carbon Monoxide Replacement Required"},
                {Alarm_CarbonMonoxide_UnknownEvent,              "Unknown"                             },
        }
    },
    {Alarm_CarbonDioxide,
        {
                {Alarm_CarbonDioxide_EventInactive,              "Carbon Dioxide Alarm Inactive"      },
                {Alarm_CarbonDioxide_CO2Detected,                "Carbon Dioxide"                     },
                {Alarm_CarbonDioxide_CO2DetectedUnknownLocation, "Carbon Dioxide"                     },
                {Alarm_CarbonDioxide_CO2Test,                    "Carbon Dioxide Test"                },
                {Alarm_CarbonDioxide_ReplacementRequired,        "Carbon Dioxide Replacement Required"},
                {Alarm_CarbonDioxide_UnknownEvent,               "Unknown"                            },
        }
    },
    {Alarm_Heat,
        {
                {Alarm_Heat_EventInactive,                       "Heat Alarm Inactive"   },
                {Alarm_Heat_OverheatDetected,                    "Heat"                  },
                {Alarm_Heat_OverheatDetectedUnknownLocation,     "Heat"                  },
                {Alarm_Heat_RapidTemperatureRise,                "Rapid Temperature Rise"},
                {Alarm_Heat_RapidTemperatureRiseUnknownLocation, "Rapid Temperature Rise"},
                {Alarm_Heat_UnderHeatDetected,                   "Underheat"             },
                {Alarm_Heat_UnderHeatDetectedUnknownLocation,    "Underheat"             },
                {Alarm_Heat_UnknownEvent,                        "Unknown"               },
        }
    },
    {Alarm_Flood,
       {
               {Alarm_Flood_EventInactive,                    "Flood Alarm Inactive"},
               {Alarm_Flood_WaterLeakDetected,                "Flood"               },
               {Alarm_Flood_WaterLeakDetectedUnknownLocation, "Flood"               },
               {Alarm_Flood_WaterLevelDropped,                "Water Level Dropped" },
               {Alarm_Flood_WaterLevelDroppedUnknownLocation, "Water Level Dropped" },
               {Alarm_Flood_ReplaceWaterFilter,               "Replace Water Filter"},
               {Alarm_Flood_UnknownEvent,                     "Unknown"             },
       }
   },
   {Alarm_Access_Control,
       {
               {Alarm_Access_Control_EventInactive,                                               "Access Control Inactive"                                              },
               {Alarm_Access_Control_ManualLockOperation,                                         "Manual Lock Operation"                                                },
               {Alarm_Access_Control_ManualUnlockOperation,                                       "Manual Unlock Operation"                                              },
               {Alarm_Access_Control_RFLockOperation,                                             "RF Lock Operation"                                                    },
               {Alarm_Access_Control_RFUnlockOperation,                                           "RF Unlock Operation"                                                  },
               {Alarm_Access_Control_KeypadLockOperation,                                         "Keypad Lock Operation"                                                },
               {Alarm_Access_Control_KeypadUnlockOperation,                                       "Keypad Unlock Operation"                                              },
               {Alarm_Access_Control_ManualNotFullyLockedOperation,                               "Manual Not Fully Locked Operation"                                    },
               {Alarm_Access_Control_RFNotFullyLockedOperation,                                   "RF Not Fully Locked Operation"                                        },
               {Alarm_Access_Control_AutoLockLockedOperation,                                     "Auto Lock Locked Operation"                                           },
               {Alarm_Access_Control_AutoLockNotFullyOperation,                                   "Auto Lock Not Fully Operation"                                        },
               {Alarm_Access_Control_LockJammed,                                                  "Lock Jammed"                                                          },
               {Alarm_Access_Control_AllUserCodesDeleted,                                         "All user codes deleted"                                               },
               {Alarm_Access_Control_SingleUserCodeDeleted,                                       "Single user code deleted"                                             },
               {Alarm_Access_Control_NewUserCodeAdded,                                            "New user code added"                                                  },
               {Alarm_Access_Control_NewUserCodeNotAddedDueToDuplicateCode,                       "New user code not added due to duplicate code"                        },
               {Alarm_Access_Control_KeypadTemporaryDisabled,                                     "Keypad temporary disabled"                                            },
               {Alarm_Access_Control_KeypadBusy,                                                  "Keypad busy"                                                          },
               {Alarm_Access_Control_NewProgramCodeEnteredUniqueCodeForLockConfiguraton,          "New Program code Entered - Unique code for lock configuration"        },
               {Alarm_Access_Control_ManuallyEnterUserAccessCodeExceedsCodeLimit,                 "Manually Enter user Access code exceeds code limit"                   },
               {Alarm_Access_Control_UnlockByRFWithInvalidUserCode,                               "Unlock By RF with invalid user code"                                  },
               {Alarm_Access_Control_LockedByRFWithInvalidUserCodes,                              "Locked by RF with invalid user codes"                                 },
               {Alarm_Access_Control_WindowDoorIsOpen,                                            "Window/Door is open"                                                  },
               {Alarm_Access_Control_WindowDoorIsClosed,                                          "Window/Door is closed"                                                },
               {Alarm_Access_Control_BarrierPerformingInitializationProcess,                      "Barrier performing Initialization process"                            },
               {Alarm_Access_Control_BarrierOperationForceHasBeenExceeded,                        "Barrier operation force has been exceeded."                           },
               {Alarm_Access_Control_BarrierMotorHasExceededManufacturersOperationalTimeLimit,    "Barrier motor has exceeded manufacturer’s operational time limit"     },
               {Alarm_Access_Control_BarrierOperationHasExceededPhysicalMechanicalLimits,         "Barrier operation has exceeded physical mechanical limits."           },
               {Alarm_Access_Control_BarrierUnableToPerformRequestedOperationDueToUlRequirements, "Barrier unable to perform requested operation due to UL requirements."},
               {Alarm_Access_Control_BarrierUnattendedOperationHasBeenDisabledPerUlRequirements,  "Barrier Unattended operation has been disabled per UL requirements."  },
               {Alarm_Access_Control_BarrierFailedToPerformRequestedOperationDeviceMalfunction,   "Barrier failed to perform Requested operation, device malfunction"    },
               {Alarm_Access_Control_BarrierVacationMode,                                         "Barrier Vacation Mode"                                                },
               {Alarm_Access_Control_BarrierSafetyBeamObstacle,                                   "Barrier Safety Beam Obstacle"                                         },
               {Alarm_Access_Control_BarrierSensorNotDetectedSupervisoryError,                    "Barrier Sensor Not Detected / Supervisory Error"                      },
               {Alarm_Access_Control_BarrierSensorLowBatteryWarning,                              "Barrier Sensor Low Battery Warning"                                   },
               {Alarm_Access_Control_BarrierDetectedShortInWallStationWires,                      "Barrier detected short in Wall Station wires"                         },
               {Alarm_Access_Control_BarrierAssociatedWithNonZWaveRemoteControl,                  "Barrier associated with non-Z-wave remote control"                    },
               {Alarm_Access_Control_UnknownEvent,                                                "Unknown"                                                              }
       }
   },
   {Alarm_Burglar,
       {
               {Alarm_Burglar_EventInactive,                  "Burglar Alarm Inactive"},
               {Alarm_Burglar_Intrusion,                      "Intrusion"             },
               {Alarm_Burglar_IntrusionUnknownLocation,       "Intrusion"             },
               {Alarm_Burglar_TamperingProductCoverRemoved,   "Tamper"                },
               {Alarm_Burglar_TamperingInvalidCode,           "Tamper Invalid Code"   },
               {Alarm_Burglar_GlassBreakage,                  "Glass Breakage"        },
               {Alarm_Burglar_GlassBreakageUnknownLocation,   "Glass Breakage"        },
               {Alarm_Burglar_MotionDetection,                "Motion"                },
               {Alarm_Burglar_MotionDetectionUnknownLocation, "Motion"                },
               {Alarm_Smoke_UnknownEvent,                     "Unknown"               },

       }
   },
   {Alarm_Power_Management,
       {
               {Alarm_Power_Management_EventInactive,         "Power Management Alarm Inactive"},
               {Alarm_Power_Management_PowerApplied,          "Power Applied"                  },
               {Alarm_Power_Management_ACMainsDisconnected,   "AC Mains Disconnected"          },
               {Alarm_Power_Management_ACMainsReconnected,    "AC Mains re-connected"          },
               {Alarm_Power_Management_SurgeDetected,         "Surge detected"                 },
               {Alarm_Power_Management_VoltageDropOrDrift,    "Voltage Drop/Drift"             },
               {Alarm_Power_Management_OvercurrentDetected,   "Over-current detected"          },
               {Alarm_Power_Management_OvervoltageDetected,   "Over-voltage detected"          },
               {Alarm_Power_Management_OverloadDetected,      "Over-load detected"             },
               {Alarm_Power_Management_LoadError,             "Load error"                     },
               {Alarm_Power_Management_ReplaceBatterySoon,    "Replace battery soon"           },
               {Alarm_Power_Management_ReplaceBatteryNow,     "Replace battery now"            },
               {Alarm_Power_Management_BatteryIsCharging,     "Battery is charging"            },
               {Alarm_Power_Management_BatteryIsFullyCharged, "Battery is fully charged"       },
               {Alarm_Power_Management_ChargeBatterySoon,     "Charge battery soon"            },
               {Alarm_Power_Management_ChargeBatteryNow,      "Charge battery now!"            },
               {Alarm_Power_Management_UnknownEvent,          "Unknown"                        },

       }
   },
   {Alarm_System,
          {
                  {Alarm_System_EventInactive,                                              "System Alarm Inactive"                                             },
                  {Alarm_System_SystemHardwareFailure,                                      "System Hardware Failure"                                           },
                  {Alarm_System_SystemSoftwareFailure,                                      "System Software Failure"                                           },
                  {Alarm_System_SystemHardwareFailureWithManufacturerProprietaryFaiureCode, "System Hardware Failure With manufacturer proprietary failure code"},
                  {Alarm_System_SystemSoftwareFailureWithManufacturerProprietaryFaiureCode, "System Software Failure With manufacturer proprietary failure code"},
                  {Alarm_System_UnknownEvent,                                               "Unknown"                                                           },
          }
   },
   {Alarm_Emergency,
          {
                  {Alarm_Emergency_EventInactive,         "Emergency Alarm Inactive"},
                  {Alarm_Emergency_ContactPolice,         "Contact Police"          },
                  {Alarm_Emergency_ContactFireService,    "Contact Fire Service"    },
                  {Alarm_Emergency_ContactMedicalService, "Contact Medical Service" },
                  {Alarm_Emergency_UnknownEvent,          "Unknown"                 },
          }
   },
   {Alarm_Clock,
          {
                  {Alarm_Clock_EventInactive, "Clock Alarm Inactive"},
                  {Alarm_Clock_WakeUpAlert,   "Wake Up Alert"       },
                  {Alarm_Clock_TimerEnded,    "Timer Ended"         },
                  {Alarm_Clock_TimeRemaining, "Time remaining"      },
                  {Alarm_Clock_UnknownEvent,  "Unknown"             },
          }
   },
   {Alarm_Appliance,
          {
                  {Alarm_Appliance_EventInactive,                 "Appliance Alarm Inactive"         },
                  {Alarm_Appliance_ProgramStarted,                "Program started"                  },
                  {Alarm_Appliance_ProgramInProgress,             "Program in progress"              },
                  {Alarm_Appliance_ProgramCompleted,              "Program completed"                },
                  {Alarm_Appliance_PreplaceMainFilter,            "Replace main filter"              },
                  {Alarm_Appliance_FailureToSetTargetTemperature, "Failure to set target temperature"},
                  {Alarm_Appliance_SupplyingWater,                "Supplying water"                  },
                  {Alarm_Appliance_WaterSupplyFailure,            "Water supply failure"             },
                  {Alarm_Appliance_Boiling,                       "Boiling"                          },
                  {Alarm_Appliance_BoilingFailure,                "Boiling failure"                  },
                  {Alarm_Appliance_Washing,                       "Washing"                          },
                  {Alarm_Appliance_WashingFailure,                "Washing failure"                  },
                  {Alarm_Appliance_Rinsing,                       "Rinsing"                          },
                  {Alarm_Appliance_RinsingFailure,                "Rinsing failure"                  },
                  {Alarm_Appliance_Draining,                      "Draining"                         },
                  {Alarm_Appliance_DrainingFailure,               "failure"                          },
                  {Alarm_Appliance_Spinning,                      "Spinning"                         },
                  {Alarm_Appliance_SpinningFailure,               "Spinning failure"                 },
                  {Alarm_Appliance_Drying,                        "Drying"                           },
                  {Alarm_Appliance_DryingFailure,                 "Drying failure"                   },
                  {Alarm_Appliance_FanFailure,                    "Fan failure"                      },
                  {Alarm_Appliance_CompressorFailure,             "Compressor failure"               },
                  {Alarm_Appliance_UnknownEvent,                  "Unknown"                          },
          }
   },
   {Alarm_HomeHealth,
          {
                  {Alarm_HomeHealth_EventInactive,                "Home Health Alarm Inactive"     },
                  {Alarm_HomeHealth_LeavingBed,                   "Leaving Bed"                    },
                  {Alarm_HomeHealth_SittingOnBed,                 "Sitting on bed"                 },
                  {Alarm_HomeHealth_LyingOnBed,                   "Lying on bed"                   },
                  {Alarm_HomeHealth_PostureChanged,               "Posture changed"                },
                  {Alarm_HomeHealth_SittingOnEdgeOfBed,           "Sitting on edge of bed"         },
                  {Alarm_HomeHealth_VolatileOrganicCompoundLevel, "Volatile Organic Compound level"},
                  {Alarm_Clock_UnknownEvent,                      "Unknown"                        },

          }
   },
};


//-----------------------------------------------------------------------------
// <WakeUp::WakeUp>
// Constructor
//-----------------------------------------------------------------------------
Alarm::Alarm
(
		uint32 const _homeId,
		uint8 const _nodeId
):
CommandClass( _homeId, _nodeId )
{
	SetStaticRequest( StaticRequest_Values );
}


//-----------------------------------------------------------------------------
// <Alarm::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Alarm::RequestState
(
		uint32 const _requestFlags,
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		if( GetVersion() > 1 )
		{
			// Request the supported alarm types
			Msg* msg = new Msg( "AlarmCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( AlarmCmd_SupportedGet );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		}
	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool Alarm::RequestValue
(
		uint32 const _requestFlags,
		uint8 const _dummy1,	// = 0 (not used)
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if( IsGetSupported() )
	{
		if( GetVersion() == 1 )
		{
			Msg* msg = new Msg( "AlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( AlarmCmd_Get );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		}
		else
		{
			bool res = false;
			for( uint8 i = 0; i < Alarm_Count; i++ )
			{
				if( Value* value = GetValue( _instance, i + 3 ) ) {
					value->Release();
					Msg* msg = new Msg( "AlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
					msg->SetInstance( this, _instance );
					msg->Append( GetNodeId() );
					msg->Append( GetVersion() == 2 ? 4 : 5);
					msg->Append( GetCommandClassId() );
					msg->Append( AlarmCmd_Get );
					msg->Append( 0x00); // ? proprietary alarm ?
					msg->Append( i );
					if( GetVersion() > 2 )
						msg->Append(0x01); //get first event of type.
					msg->Append( GetDriver()->GetTransmitOptions() );
					GetDriver()->SendMsg( msg, _queue );
					res = true;
				}
			}
			return res;
		}
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "AlarmCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Alarm::HandleMsg
(
		uint8 const* _data,
		uint32 const _length,
		uint32 const _instance	// = 1
)
{
	if (AlarmCmd_Report == (AlarmCmd)_data[0])
	{
		// We have received a report from the Z-Wave device
		if( GetVersion() == 1 )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received Alarm report: type=%d, level=%d", _data[1], _data[2] );
		}
		else
		{
			string alarm_type =  ( _data[5] < Alarm_Count ) ? c_alarmTypeName[_data[5]] : "Unknown type";

			Log::Write( LogLevel_Info, GetNodeId(), "Received Alarm report: type=%d, level=%d, sensorSrcID=%d, type:%s event:%d, status=%d",
							_data[1], _data[2], _data[3], alarm_type.c_str(), _data[6], _data[4] );
		}

		ValueByte* value;
		if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Type ) )) )
		{
			value->OnValueRefreshed( _data[1] );
			value->Release();
		}
		if ( GetVersion() <= 2)
		{
		    if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Level ) )) )
		    {
		        value->OnValueRefreshed( _data[2] );
		        value->Release();
		    }
		}

		// With Version=2, the data has more detailed information about the alarm
		if(( GetVersion() == 2 ) && ( _length >= 7  ))
		{
			if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_SourceNodeId ) )) )
			{
				value->OnValueRefreshed( _data[3] );
				value->Release();
			}

			if( (value = static_cast<ValueByte*>( GetValue( _instance, _data[5]+3 ) )) )
			{
				value->OnValueRefreshed( _data[6] );
				value->Release();
			}
		}
		if ( GetVersion() >= 3 )
		{
		    // the index is as follows:
		    // alarmPrefixCount[notificationType]+event+3
		    // If i get everything right, the event tells the application "here, something happend", and a Event_Inacativ message clears it" (some devices seem to do it with 0xFE instead of 0x00 -.-)
		    // there is no "Event (e.g. motion alarm) is now active and now motion alarm is inactive...
		    uint8 notificationStatus = _data[4];
		    uint8 notificationType = _data[5];
		    uint8 event = _data[6];
		    ValueByte* value;

		    // this is workaround for aeotec multisensor gen 5
		    // this device sends motion on and off with the notification status, but the status is not exposed to the apps
		    // so i abuse the AlarmLevel from version 1 and 2.
	        if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Level ) )) )
	        {
	            value->OnValueRefreshed( notificationStatus );
	            value->Release();
	        }
		    if ( event == 0x00 || event == 0xFE )
		    {
		        //clear all events untill i find a device that does it right \o/
		        for (uint8 i=0; i< alarmCounts[notificationType]; ++i)
		        {
		            ValueByte* value;
		            if( (value = static_cast<ValueByte*>( GetValue( _instance, alarmPrefixCount[notificationType]+i+3 ) )) )
                    {
                        value->OnValueRefreshed( 0 );
                        value->Release();
                    }
		        }
		    }
		    else
		    {
                if( (value = static_cast<ValueByte*>( GetValue( _instance, alarmPrefixCount[notificationType]+event+3 ) )) )
                {
                    value->OnValueRefreshed( 1 );
                    value->Release();
                }
		    }
		}
		return true;
	}

	else if( AlarmCmd_SupportedReport == (AlarmCmd)_data[0] )
	{
		if( Node* node = GetNodeUnsafe() )
		{
            // We have received the supported alarm types from the Z-Wave device
            Log::Write( LogLevel_Info, GetNodeId(), "Received supported alarm types" );

            node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_SourceNodeId, "SourceNodeId", "", true, false, 0, 0 );
            Log::Write( LogLevel_Info, GetNodeId(), "    Added alarm SourceNodeId" );

            // Parse the data for the supported alarm types
            uint8 numBytes = _data[1];
            for( uint32 i=0; i<numBytes; ++i )
            {
                for( int32 bit=0; bit<8; ++bit )
                {
                    if( ( _data[i+2] & (1<<bit) ) != 0 )
                    {
                        int32 index = (int32)(i<<3) + bit;
                        if( index < Alarm_Count )
                        {
                            //this is version 1 or 2
                            if ( GetVersion() <= 2 ) {
                                node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, index+3, c_alarmTypeName[index], "", true, false, 0, 0 );
                                Log::Write( LogLevel_Info, GetNodeId(), "    Added alarm type: %s", c_alarmTypeName[index] );
                            }
                            //this is Version 3 or higher, it now supports to ask for supported events for each notification Type
                            else if ( GetVersion() >= 3 )
                            {
                                // Request the supported events
                                Msg* msg = new Msg( "AlarmCmd_SupportedEventGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
                                msg->SetInstance( this, _instance );
                                msg->Append( GetNodeId() );
                                msg->Append( 3 );
                                msg->Append( GetCommandClassId() );
                                msg->Append( AlarmCmd_SupportedEventGet );
                                msg->Append( index );
                                msg->Append( GetDriver()->GetTransmitOptions() );
                                GetDriver()->SendMsg( msg, OpenZWave::Driver::MsgQueue_Query );
                            }
                        } else {
                            Log::Write( LogLevel_Info, GetNodeId(), "    Unknown alarm type: %d", index );
                        }
                    }
                }
            }
		}

		ClearStaticRequest( StaticRequest_Values );
		return true;
	}
	else if( AlarmCmd_SupportedEventReport == (AlarmCmd)_data[0] )
    {
	    if( Node* node = GetNodeUnsafe() )
        {
            uint8 numBytes = _data[2] & 0x1F;
            Log::Write( LogLevel_Detail, GetNodeId(), "    received SupportedEventReport for Notification Type: %d - %s", _data[1], c_alarmTypeName[_data[1]]);
            for( uint32 i=0; i<numBytes; ++i )
            {
                for( int32 bit=0; bit<8; ++bit )
                {
                    if( ( _data[3+i] & (1<<bit) ) != 0 )
                    {
                        int32 index = (int32)(i<<3) + bit;
                        Log::Write( LogLevel_Detail, GetNodeId(), "    supported Event Index: %d name: %s ", index , m_EventTypeName[_data[1]][index]);
                        node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, alarmPrefixCount[_data[1]]+index+3,  m_EventTypeName[_data[1]][index], "", true, false, 0, 0 );
                    }
                }
            }
        }

//	    int number_of_bytes = _data[2] & 0x1F;
//	    vector<int> supported_alarm_bits ;
//	    for (int b = 0 ; b <=7; b++) {
//                int c = ((_data[2+i] >> b ) & 1);
//                spdlog::get(CALogClassZwave)->debug("Bit {0:02}: {1:02} ({2:02x})",alarm_bit,c , BinaryData[3+i]);
//                if ( c == 1 ) {
//                    supported_alarm_bits.push_back(alarm_bit);
//                }
//                alarm_bit++;
//            }
//	    Log::Write( LogLevel_Info, GetNodeId(), "Received Alarm report: type=%d, level=%d, sensorSrcID=%d, type:%s event:%d, status=%d",
//	                                _data[1], _data[2], _data[3], alarm_type.c_str(), _data[6], _data[4] );

	    return true;
    }

	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Alarm::CreateVars
(
		uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type, "Alarm Type", "", true, false, 0, 0 );
		node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Level, "Alarm Level", "", true, false, 0, 0 );
		if ( GetVersion() >= 3)
		{
		    node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Level, "Alarm Level", "", true, false, 0, 0 );
		}
	}
}

