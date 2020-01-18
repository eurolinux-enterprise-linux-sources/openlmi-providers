# -*- encoding: utf-8 -*-
# Software Management Providers
#
# Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

"""
Just a common functionality related to class CIM_Error.
"""

import pywbem

from lmi.providers import cmpi_logging
from lmi.software import util

class Values(object):
    class ErrorSourceFormat(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        CIMObjectPath = pywbem.Uint16(2)
        # DMTF_Reserved = ..
        _reverse_map = {
                0 : 'Unknown',
                1 : 'Other',
                2 : 'CIMObjectPath'
        }

    class ErrorType(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Communications_Error = pywbem.Uint16(2)
        Quality_of_Service_Error = pywbem.Uint16(3)
        Software_Error = pywbem.Uint16(4)
        Hardware_Error = pywbem.Uint16(5)
        Environmental_Error = pywbem.Uint16(6)
        Security_Error = pywbem.Uint16(7)
        Oversubscription_Error = pywbem.Uint16(8)
        Unavailable_Resource_Error = pywbem.Uint16(9)
        Unsupported_Operation_Error = pywbem.Uint16(10)
        # DMTF_Reserved = ..
        _reverse_map = {
                0  : 'Unknown',
                1  : 'Other',
                2  : 'Communications Error',
                3  : 'Quality of Service Error',
                4  : 'Software Error',
                5  : 'Hardware Error',
                6  : 'Environmental Error',
                7  : 'Security Error',
                8  : 'Oversubscription Error',
                9  : 'Unavailable Resource Error',
                10 : 'Unsupported Operation Error'
        }

    class CIMStatusCode(object):
        CIM_ERR_FAILED = pywbem.Uint32(1)
        CIM_ERR_ACCESS_DENIED = pywbem.Uint32(2)
        CIM_ERR_INVALID_NAMESPACE = pywbem.Uint32(3)
        CIM_ERR_INVALID_PARAMETER = pywbem.Uint32(4)
        CIM_ERR_INVALID_CLASS = pywbem.Uint32(5)
        CIM_ERR_NOT_FOUND = pywbem.Uint32(6)
        CIM_ERR_NOT_SUPPORTED = pywbem.Uint32(7)
        CIM_ERR_CLASS_HAS_CHILDREN = pywbem.Uint32(8)
        CIM_ERR_CLASS_HAS_INSTANCES = pywbem.Uint32(9)
        CIM_ERR_INVALID_SUPERCLASS = pywbem.Uint32(10)
        CIM_ERR_ALREADY_EXISTS = pywbem.Uint32(11)
        CIM_ERR_NO_SUCH_PROPERTY = pywbem.Uint32(12)
        CIM_ERR_TYPE_MISMATCH = pywbem.Uint32(13)
        CIM_ERR_QUERY_LANGUAGE_NOT_SUPPORTED = pywbem.Uint32(14)
        CIM_ERR_INVALID_QUERY = pywbem.Uint32(15)
        CIM_ERR_METHOD_NOT_AVAILABLE = pywbem.Uint32(16)
        CIM_ERR_METHOD_NOT_FOUND = pywbem.Uint32(17)
        CIM_ERR_UNEXPECTED_RESPONSE = pywbem.Uint32(18)
        CIM_ERR_INVALID_RESPONSE_DESTINATION = pywbem.Uint32(19)
        CIM_ERR_NAMESPACE_NOT_EMPTY = pywbem.Uint32(20)
        CIM_ERR_INVALID_ENUMERATION_CONTEXT = pywbem.Uint32(21)
        CIM_ERR_INVALID_OPERATION_TIMEOUT = pywbem.Uint32(22)
        CIM_ERR_PULL_HAS_BEEN_ABANDONED = pywbem.Uint32(23)
        CIM_ERR_PULL_CANNOT_BE_ABANDONED = pywbem.Uint32(24)
        CIM_ERR_FILTERED_ENUMERATION_NOT_SUPPORTED = pywbem.Uint32(25)
        CIM_ERR_CONTINUATION_ON_ERROR_NOT_SUPPORTED = pywbem.Uint32(26)
        CIM_ERR_SERVER_LIMITS_EXCEEDED = pywbem.Uint32(27)
        CIM_ERR_SERVER_IS_SHUTTING_DOWN = pywbem.Uint32(28)
        CIM_ERR_QUERY_FEATURE_NOT_SUPPORTED = pywbem.Uint32(29)
        # DMTF_Reserved = ..
        _reverse_map = {
                1  : 'CIM_ERR_FAILED',
                2  : 'CIM_ERR_ACCESS_DENIED',
                3  : 'CIM_ERR_INVALID_NAMESPACE',
                4  : 'CIM_ERR_INVALID_PARAMETER',
                5  : 'CIM_ERR_INVALID_CLASS',
                6  : 'CIM_ERR_NOT_FOUND',
                7  : 'CIM_ERR_NOT_SUPPORTED',
                8  : 'CIM_ERR_CLASS_HAS_CHILDREN',
                9  : 'CIM_ERR_CLASS_HAS_INSTANCES',
                10 : 'CIM_ERR_INVALID_SUPERCLASS',
                11 : 'CIM_ERR_ALREADY_EXISTS',
                12 : 'CIM_ERR_NO_SUCH_PROPERTY',
                13 : 'CIM_ERR_TYPE_MISMATCH',
                14 : 'CIM_ERR_QUERY_LANGUAGE_NOT_SUPPORTED',
                15 : 'CIM_ERR_INVALID_QUERY',
                16 : 'CIM_ERR_METHOD_NOT_AVAILABLE',
                17 : 'CIM_ERR_METHOD_NOT_FOUND',
                18 : 'CIM_ERR_UNEXPECTED_RESPONSE',
                19 : 'CIM_ERR_INVALID_RESPONSE_DESTINATION',
                20 : 'CIM_ERR_NAMESPACE_NOT_EMPTY',
                21 : 'CIM_ERR_INVALID_ENUMERATION_CONTEXT',
                22 : 'CIM_ERR_INVALID_OPERATION_TIMEOUT',
                23 : 'CIM_ERR_PULL_HAS_BEEN_ABANDONED',
                24 : 'CIM_ERR_PULL_CANNOT_BE_ABANDONED',
                25 : 'CIM_ERR_FILTERED_ENUMERATION_NOT_SUPPORTED',
                26 : 'CIM_ERR_CONTINUATION_ON_ERROR_NOT_SUPPORTED',
                27 : 'CIM_ERR_SERVER_LIMITS_EXCEEDED',
                28 : 'CIM_ERR_SERVER_IS_SHUTTING_DOWN',
                29 : 'CIM_ERR_QUERY_FEATURE_NOT_SUPPORTED'
        }

    class PerceivedSeverity(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Information = pywbem.Uint16(2)
        Degraded_Warning = pywbem.Uint16(3)
        Minor = pywbem.Uint16(4)
        Major = pywbem.Uint16(5)
        Critical = pywbem.Uint16(6)
        Fatal_NonRecoverable = pywbem.Uint16(7)
        # DMTF_Reserved = ..
        _reverse_map = {
                0 : 'Unknown',
                1 : 'Other',
                2 : 'Information',
                3 : 'Degraded/Warning',
                4 : 'Minor',
                5 : 'Major',
                6 : 'Critical',
                7 : 'Fatal/NonRecoverable'
        }

    class ProbableCause(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Adapter_Card_Error = pywbem.Uint16(2)
        Application_Subsystem_Failure = pywbem.Uint16(3)
        Bandwidth_Reduced = pywbem.Uint16(4)
        Connection_Establishment_Error = pywbem.Uint16(5)
        Communications_Protocol_Error = pywbem.Uint16(6)
        Communications_Subsystem_Failure = pywbem.Uint16(7)
        Configuration_Customization_Error = pywbem.Uint16(8)
        Congestion = pywbem.Uint16(9)
        Corrupt_Data = pywbem.Uint16(10)
        CPU_Cycles_Limit_Exceeded = pywbem.Uint16(11)
        Dataset_Modem_Error = pywbem.Uint16(12)
        Degraded_Signal = pywbem.Uint16(13)
        DTE_DCE_Interface_Error = pywbem.Uint16(14)
        Enclosure_Door_Open = pywbem.Uint16(15)
        Equipment_Malfunction = pywbem.Uint16(16)
        Excessive_Vibration = pywbem.Uint16(17)
        File_Format_Error = pywbem.Uint16(18)
        Fire_Detected = pywbem.Uint16(19)
        Flood_Detected = pywbem.Uint16(20)
        Framing_Error = pywbem.Uint16(21)
        HVAC_Problem = pywbem.Uint16(22)
        Humidity_Unacceptable = pywbem.Uint16(23)
        I_O_Device_Error = pywbem.Uint16(24)
        Input_Device_Error = pywbem.Uint16(25)
        LAN_Error = pywbem.Uint16(26)
        Non_Toxic_Leak_Detected = pywbem.Uint16(27)
        Local_Node_Transmission_Error = pywbem.Uint16(28)
        Loss_of_Frame = pywbem.Uint16(29)
        Loss_of_Signal = pywbem.Uint16(30)
        Material_Supply_Exhausted = pywbem.Uint16(31)
        Multiplexer_Problem = pywbem.Uint16(32)
        Out_of_Memory = pywbem.Uint16(33)
        Output_Device_Error = pywbem.Uint16(34)
        Performance_Degraded = pywbem.Uint16(35)
        Power_Problem = pywbem.Uint16(36)
        Pressure_Unacceptable = pywbem.Uint16(37)
        Processor_Problem__Internal_Machine_Error_ = pywbem.Uint16(38)
        Pump_Failure = pywbem.Uint16(39)
        Queue_Size_Exceeded = pywbem.Uint16(40)
        Receive_Failure = pywbem.Uint16(41)
        Receiver_Failure = pywbem.Uint16(42)
        Remote_Node_Transmission_Error = pywbem.Uint16(43)
        Resource_at_or_Nearing_Capacity = pywbem.Uint16(44)
        Response_Time_Excessive = pywbem.Uint16(45)
        Retransmission_Rate_Excessive = pywbem.Uint16(46)
        Software_Error = pywbem.Uint16(47)
        Software_Program_Abnormally_Terminated = pywbem.Uint16(48)
        Software_Program_Error__Incorrect_Results_ = pywbem.Uint16(49)
        Storage_Capacity_Problem = pywbem.Uint16(50)
        Temperature_Unacceptable = pywbem.Uint16(51)
        Threshold_Crossed = pywbem.Uint16(52)
        Timing_Problem = pywbem.Uint16(53)
        Toxic_Leak_Detected = pywbem.Uint16(54)
        Transmit_Failure = pywbem.Uint16(55)
        Transmitter_Failure = pywbem.Uint16(56)
        Underlying_Resource_Unavailable = pywbem.Uint16(57)
        Version_Mismatch = pywbem.Uint16(58)
        Previous_Alert_Cleared = pywbem.Uint16(59)
        Login_Attempts_Failed = pywbem.Uint16(60)
        Software_Virus_Detected = pywbem.Uint16(61)
        Hardware_Security_Breached = pywbem.Uint16(62)
        Denial_of_Service_Detected = pywbem.Uint16(63)
        Security_Credential_Mismatch = pywbem.Uint16(64)
        Unauthorized_Access = pywbem.Uint16(65)
        Alarm_Received = pywbem.Uint16(66)
        Loss_of_Pointer = pywbem.Uint16(67)
        Payload_Mismatch = pywbem.Uint16(68)
        Transmission_Error = pywbem.Uint16(69)
        Excessive_Error_Rate = pywbem.Uint16(70)
        Trace_Problem = pywbem.Uint16(71)
        Element_Unavailable = pywbem.Uint16(72)
        Element_Missing = pywbem.Uint16(73)
        Loss_of_Multi_Frame = pywbem.Uint16(74)
        Broadcast_Channel_Failure = pywbem.Uint16(75)
        Invalid_Message_Received = pywbem.Uint16(76)
        Routing_Failure = pywbem.Uint16(77)
        Backplane_Failure = pywbem.Uint16(78)
        Identifier_Duplication = pywbem.Uint16(79)
        Protection_Path_Failure = pywbem.Uint16(80)
        Sync_Loss_or_Mismatch = pywbem.Uint16(81)
        Terminal_Problem = pywbem.Uint16(82)
        Real_Time_Clock_Failure = pywbem.Uint16(83)
        Antenna_Failure = pywbem.Uint16(84)
        Battery_Charging_Failure = pywbem.Uint16(85)
        Disk_Failure = pywbem.Uint16(86)
        Frequency_Hopping_Failure = pywbem.Uint16(87)
        Loss_of_Redundancy = pywbem.Uint16(88)
        Power_Supply_Failure = pywbem.Uint16(89)
        Signal_Quality_Problem = pywbem.Uint16(90)
        Battery_Discharging = pywbem.Uint16(91)
        Battery_Failure = pywbem.Uint16(92)
        Commercial_Power_Problem = pywbem.Uint16(93)
        Fan_Failure = pywbem.Uint16(94)
        Engine_Failure = pywbem.Uint16(95)
        Sensor_Failure = pywbem.Uint16(96)
        Fuse_Failure = pywbem.Uint16(97)
        Generator_Failure = pywbem.Uint16(98)
        Low_Battery = pywbem.Uint16(99)
        Low_Fuel = pywbem.Uint16(100)
        Low_Water = pywbem.Uint16(101)
        Explosive_Gas = pywbem.Uint16(102)
        High_Winds = pywbem.Uint16(103)
        Ice_Buildup = pywbem.Uint16(104)
        Smoke = pywbem.Uint16(105)
        Memory_Mismatch = pywbem.Uint16(106)
        Out_of_CPU_Cycles = pywbem.Uint16(107)
        Software_Environment_Problem = pywbem.Uint16(108)
        Software_Download_Failure = pywbem.Uint16(109)
        Element_Reinitialized = pywbem.Uint16(110)
        Timeout = pywbem.Uint16(111)
        Logging_Problems = pywbem.Uint16(112)
        Leak_Detected = pywbem.Uint16(113)
        Protection_Mechanism_Failure = pywbem.Uint16(114)
        Protecting_Resource_Failure = pywbem.Uint16(115)
        Database_Inconsistency = pywbem.Uint16(116)
        Authentication_Failure = pywbem.Uint16(117)
        Breach_of_Confidentiality = pywbem.Uint16(118)
        Cable_Tamper = pywbem.Uint16(119)
        Delayed_Information = pywbem.Uint16(120)
        Duplicate_Information = pywbem.Uint16(121)
        Information_Missing = pywbem.Uint16(122)
        Information_Modification = pywbem.Uint16(123)
        Information_Out_of_Sequence = pywbem.Uint16(124)
        Key_Expired = pywbem.Uint16(125)
        Non_Repudiation_Failure = pywbem.Uint16(126)
        Out_of_Hours_Activity = pywbem.Uint16(127)
        Out_of_Service = pywbem.Uint16(128)
        Procedural_Error = pywbem.Uint16(129)
        Unexpected_Information = pywbem.Uint16(130)
        # DMTF_Reserved = ..
        _reverse_map = {
                0   : 'Unknown',
                1   : 'Other',
                2   : 'Adapter/Card Error',
                3   : 'Application Subsystem Failure',
                4   : 'Bandwidth Reduced',
                5   : 'Connection Establishment Error',
                6   : 'Communications Protocol Error',
                7   : 'Communications Subsystem Failure',
                8   : 'Configuration/Customization Error',
                9   : 'Congestion',
                10  : 'Corrupt Data',
                11  : 'CPU Cycles Limit Exceeded',
                12  : 'Dataset/Modem Error',
                13  : 'Degraded Signal',
                14  : 'DTE-DCE Interface Error',
                15  : 'Enclosure Door Open',
                16  : 'Equipment Malfunction',
                17  : 'Excessive Vibration',
                18  : 'File Format Error',
                19  : 'Fire Detected',
                20  : 'Flood Detected',
                21  : 'Framing Error',
                22  : 'HVAC Problem',
                23  : 'Humidity Unacceptable',
                24  : 'I/O Device Error',
                25  : 'Input Device Error',
                26  : 'LAN Error',
                27  : 'Non-Toxic Leak Detected',
                28  : 'Local Node Transmission Error',
                29  : 'Loss of Frame',
                30  : 'Loss of Signal',
                31  : 'Material Supply Exhausted',
                32  : 'Multiplexer Problem',
                33  : 'Out of Memory',
                34  : 'Output Device Error',
                35  : 'Performance Degraded',
                36  : 'Power Problem',
                37  : 'Pressure Unacceptable',
                38  : 'Processor Problem (Internal Machine Error)',
                39  : 'Pump Failure',
                40  : 'Queue Size Exceeded',
                41  : 'Receive Failure',
                42  : 'Receiver Failure',
                43  : 'Remote Node Transmission Error',
                44  : 'Resource at or Nearing Capacity',
                45  : 'Response Time Excessive',
                46  : 'Retransmission Rate Excessive',
                47  : 'Software Error',
                48  : 'Software Program Abnormally Terminated',
                49  : 'Software Program Error (Incorrect Results)',
                50  : 'Storage Capacity Problem',
                51  : 'Temperature Unacceptable',
                52  : 'Threshold Crossed',
                53  : 'Timing Problem',
                54  : 'Toxic Leak Detected',
                55  : 'Transmit Failure',
                56  : 'Transmitter Failure',
                57  : 'Underlying Resource Unavailable',
                58  : 'Version Mismatch',
                59  : 'Previous Alert Cleared',
                60  : 'Login Attempts Failed',
                61  : 'Software Virus Detected',
                62  : 'Hardware Security Breached',
                63  : 'Denial of Service Detected',
                64  : 'Security Credential Mismatch',
                65  : 'Unauthorized Access',
                66  : 'Alarm Received',
                67  : 'Loss of Pointer',
                68  : 'Payload Mismatch',
                69  : 'Transmission Error',
                70  : 'Excessive Error Rate',
                71  : 'Trace Problem',
                72  : 'Element Unavailable',
                73  : 'Element Missing',
                74  : 'Loss of Multi Frame',
                75  : 'Broadcast Channel Failure',
                76  : 'Invalid Message Received',
                77  : 'Routing Failure',
                78  : 'Backplane Failure',
                79  : 'Identifier Duplication',
                80  : 'Protection Path Failure',
                81  : 'Sync Loss or Mismatch',
                82  : 'Terminal Problem',
                83  : 'Real Time Clock Failure',
                84  : 'Antenna Failure',
                85  : 'Battery Charging Failure',
                86  : 'Disk Failure',
                87  : 'Frequency Hopping Failure',
                88  : 'Loss of Redundancy',
                89  : 'Power Supply Failure',
                90  : 'Signal Quality Problem',
                91  : 'Battery Discharging',
                92  : 'Battery Failure',
                93  : 'Commercial Power Problem',
                94  : 'Fan Failure',
                95  : 'Engine Failure',
                96  : 'Sensor Failure',
                97  : 'Fuse Failure',
                98  : 'Generator Failure',
                99  : 'Low Battery',
                100 : 'Low Fuel',
                101 : 'Low Water',
                102 : 'Explosive Gas',
                103 : 'High Winds',
                104 : 'Ice Buildup',
                105 : 'Smoke',
                106 : 'Memory Mismatch',
                107 : 'Out of CPU Cycles',
                108 : 'Software Environment Problem',
                109 : 'Software Download Failure',
                110 : 'Element Reinitialized',
                111 : 'Timeout',
                112 : 'Logging Problems',
                113 : 'Leak Detected',
                114 : 'Protection Mechanism Failure',
                115 : 'Protecting Resource Failure',
                116 : 'Database Inconsistency',
                117 : 'Authentication Failure',
                118 : 'Breach of Confidentiality',
                119 : 'Cable Tamper',
                120 : 'Delayed Information',
                121 : 'Duplicate Information',
                122 : 'Information Missing',
                123 : 'Information Modification',
                124 : 'Information Out of Sequence',
                125 : 'Key Expired',
                126 : 'Non-Repudiation Failure',
                127 : 'Out of Hours Activity',
                128 : 'Out of Service',
                129 : 'Procedural Error',
                130 : 'Unexpected Information'
        }

@cmpi_logging.trace_function
def make_instance(
        env,
        status_code=Values.CIMStatusCode.CIM_ERR_FAILED,
        error_type=Values.ErrorType.Software_Error,
        probable_cause=Values.ErrorType.Unknown,
        error_source=None,
        status_code_description=None,
        message=None,
        message_arguments=None,
        probable_cause_description=None,
        recommended_actions=None):
    for param in ('status_code', 'probable_cause', 'error_type'):
        if not isinstance(locals()[param], (int, long)):
            raise TypeError('%s must be integer'%param)
    if error_source is not None \
            and not isinstance(error_source, pywbem.CIMInstanceName):
        raise TypeError('error_source must be a CIMInstanceName')

    inst = pywbem.CIMInstance(classname="CIM_Error",
            path=util.new_instance_name("CIM_Error"))
    inst['CIMStatusCode'] = pywbem.Uint32(status_code)
    if status_code_description is not None:
        inst['CIMStatusCodeDescription'] = status_code_description
    inst['ErrorType'] = pywbem.Uint16(error_type)
    if error_source:
        inst['ErrorSource'] = str(error_source)
        inst['ErrorSourceFormat'] = Values.ErrorSourceFormat.CIMObjectPath
    if message is not None:
        inst['Message'] = message
    if message_arguments is not None:
        inst['MessageArguments'] = message_arguments
    inst['ProbableCause'] = pywbem.Uint16(probable_cause)
    if probable_cause_description is not None:
        inst['ProbableCauseDescription'] = probable_cause_description
    if recommended_actions is not None:
        inst['RecommendedActions'] = recommended_actions
    return inst

