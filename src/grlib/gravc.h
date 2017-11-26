//
// Copyright (C) 2017 Graeme Walker
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
///
/// \file gravc.h
///

#ifndef GR_AVC__H
#define GR_AVC__H

#include "gdef.h"
#include "gbititerator.h"
#include "gbitstream.h"
#include <string>
#include <vector>
#include <iostream>

namespace Gr
{
	/// \namespace Gr::Avc
	/// Classes for decoding H.264 parameter sets.
	/// 
	/// H.264 is the standard for Advanced Video Coding (AVC).
	/// 
	/// The AVC design separates out its configuration parameters from the
	/// stream of video images (frames), with the configuration being 
	/// communicated completely out-of-band (eg. SDP) or in distinctly-typed 
	/// network packets (SPS and PPS NALUs) or in special file segments 
	/// (eg. "avcC" boxes).
	/// 
	/// Once the parameter sets have been established the video frames can
	/// simply refer to them by their id. In the simplest case there is one 
	/// sequence parameter set (SPS) and one picture parameter set (PPS) 
	/// used by all video frames.
	/// 
	/// The job of decoding parameter sets can sometimes be delegated to a 
	/// H.264 decoder by presenting them as extra data at session setup, or
	/// by adding them to the data stream as SPS and PPS NALUs.
	/// 
	/// The H.264 standard is defined in terms of a bitstream syntax. A 
	/// bytestream format, RBSP, is also defined with a final stop bit 
	/// set to 1 followed by a set of padding bits set to 0. A RBSP bytestream 
	/// can be byte-stuffed in order that easily-identifiable markers can be 
	/// put inbetween the set of RBSP buffers that make up a higher-level 
	/// bytestream. The RBSP byte-stuffing replaces all 00-00-00, 00-00-01, 
	/// 00-00-02 or 00-00-03 sequences with 00-00-03-00, 00-00-03-01, 
	/// 00-00-03-02 or 00-00-03-03 respectively, thereby freeing up 00-00-00, 
	/// 00-00-01 and 00-00-02 to be used as inter-RBSP markers.
	/// 
	/// H.264 NALUs are made up of a single byte-stuffed RBSP buffer with a 
	/// one-byte header to identify the NALU type. When NALUs are chained 
	/// together they are separated by 00-00-01 markers, with an additional 
	/// four-byte 00-00-00-01 start-code marker at the front. Other NALU packing 
	/// schemes use NALU-length fields between the separate NALUs (mp4-avcC, 
	/// rtp-avc), or use base64 encoding of each NALU with comma separators
	/// (fmtp), but even in these cases the NALUs themselves are byte-stuffed.
	/// 
	/// \see ISO/IEC 14496-15 (non-free) 5.2.4.1.1 5.2.3 5.3.4.1.2, 
	/// and libav's ff_isom_write_avcc().
	/// 
	namespace Avc
	{
		class Configuration ;
		class Sps ;
		class Pps ;
		class Rbsp ;
		std::ostream & operator<<( std::ostream & , const Avc::Sps & ) ;
		std::ostream & operator<<( std::ostream & , const Avc::Pps & ) ;
		typedef G::bit_iterator<const char*> bit_iterator_t ;
		typedef G::bit_stream<bit_iterator_t> bit_stream_t ;
	}
}

/// \class Gr::Avc::Configuration
/// Contains AVC configuration parameters, initialised from an "avcC" file 
/// segment or from an SDP "fmtp" attribute string, held as lists of
/// SPS and PPS sub-structures. In practice there is often only one SPS 
/// and one PPS.
/// 
class Gr::Avc::Configuration
{
public:
	static Configuration fromFmtp( const std::string & fmtp ) ;
		///< Factory function taking a SDP (Session Description Protocol) "fmtp" attribute string, 
		///< something like "profile-level-id=...; ...; sprop-parameters-sets=Z00AKZpmA8==,aO48gA==".

	static Configuration fromAvcc( const std::string & binary_string ) ;
		///< Factory function taking an "avcC" binary string, including RBSP 
		///< byte-stuffing (but no start code) -- see ISO/IEC 14496-10 7.4.1.1.

	Configuration() ;
		///< A default constructor for an in-valid() object.

	bool valid() const ;
		///< Returns true if a usable object.

	std::string reason() const ;
		///< Returns the in-valid() reason.

	unsigned int nalu_length_size() const ;
		///< Returns the size of the nalu length values, typically 2.

	const std::vector<std::string> & spsList() const ;
		///< Returns a list of binary strings for the sps structures.
		///< The strings are byte-stuffed and start with the nalu type
		///< in the low bits of the first character position.

	const std::vector<std::string> & ppsList() const ;
		///< Returns a list of binary strings for the pps structures.
		///< The strings are byte-stuffed and start with the nalu type
		///< in the low bits of the first character position.

	size_t spsCount() const ;
		///< Returns sps().size(). This will be smaller than spsList.size()
		///< if there are parsing errors.

	size_t ppsCount() const ;
		///< Returns pps().size(). This will be smaller than spsList.size()
		///< if there are parsing errors.

	const Sps & sps( size_t i ) const ;
		///< Returns a reference to the i-th sps structure.

	const Pps & pps( size_t i ) const ;
		///< Returns a reference to the i-th pps structure.

	std::string nalus() const ;
		///< Returns the NALU byte-stream comprising the four-byte 00-00-00-01
		///< start-code followed by the byte-stuffed NALUs separated by the 
		///< three-byte 00-00-01 marker.

private:
	Configuration( bool , const std::string & ) ;
	void initFmtp( const std::string & ) ;
	void initAvcc( const std::string & ) ;
	void checkFmtp() ;
	void checkStream( bit_stream_t & ) ;
	void spsListParse() ;
	void ppsListParse() ;
	void checkIds() ;
	void commit() ;
	void report( const std::string & ) ;

private:
	struct AvccInfo
	{
		unsigned int configuration_version ;
		unsigned int profile ; // also in sps
		unsigned int profile_compat ;
		unsigned int level ; // also in sps
		unsigned int length_size_minus_one ; // also in sps
	} ;
	struct FmtpInfo
	{
		unsigned int profile ; // also in sps
		unsigned int level ; // also in sps
		unsigned int packetisation_mode ;
	} ;

private:
	AvccInfo m_avcc_info ;
	FmtpInfo m_fmtp_info ;
	std::string m_reason ;
	std::vector<std::string> m_sps_list ;
	std::vector<std::string> m_pps_list ;
	std::vector<Sps> m_sps ;
	std::vector<Pps> m_pps ;
} ;

/// \class Gr::Avc::Sps
/// A Sequence Parameter Set (SPS) structure.
/// \see ISO/IEC 14496-10, esp. 7.3.2.1.1
/// 
class Gr::Avc::Sps
{
public:
	explicit Sps( const std::string & binary_sps_buffer ) ;
		///< Constructor taking a binary byte-stuffed sps buffer.

	bool valid() const ;
		///< Returns true if a usable object.

	std::string reason() const ;
		///< Returns the in-valid() reason.

	void streamOut( std::ostream & ) const ;
		///< Streams out the sps info.

	unsigned int dx() const ;
		///< Returns the picture width in pixels.

	unsigned int dy() const ;
		///< Returns the picture height in pixels.

public:
	unsigned int m_nalu_type ; // 7
	unsigned int m_profile_idc ; // eg. 66
	bool m_constraint_set0_flag ; // see 7.4.2.1.1 and A.2.1
	bool m_constraint_set1_flag ; // see A.2.2
	bool m_constraint_set2_flag ;
	bool m_constraint_set3_flag ;
	bool m_constraint_set4_flag ;
	bool m_constraint_set5_flag ;
	unsigned int m_zero ; // 2 bits
	unsigned int m_level_idc ;
	unsigned int m_sequence_parameter_set_id ;
	unsigned int m_chroma_format_idc ;
	bool m_separate_colour_plane_flag ;
	unsigned int m_bit_depth_luma_minus8 ;
	unsigned int m_bit_depth_chroma_minus8 ;
	bool m_qpprime_y_zero_transform_bypass_flag ;
	bool m_seq_scaling_matrix_present_flag ;
	unsigned int m_log2_max_frame_num_minus4 ;
	unsigned int m_pic_order_cnt_type ;
	unsigned int m_log2_max_pic_order_cnt_lsb_minus4 ;
	bool m_delta_pic_order_always_zero_flag ;
	int m_offset_for_non_ref_pic ;
	int m_offset_for_top_to_bottom_field ;
	unsigned int m_num_ref_frames_in_pic_order_cnt_cycle ;
	unsigned int m_max_num_ref_frames ;
	bool m_gaps_in_frame_num_value_allowed_flag ;
	unsigned int m_pic_width_in_mbs_minus1 ;
	unsigned int m_pic_height_in_map_units_minus1 ;
	bool m_frame_mbs_only_flag ;
	bool m_mb_adaptive_frame_field_flag ;
	bool m_direct_8x8_inference_flag ;
	bool m_frame_cropping_flag ;
	unsigned int m_frame_crop_left_offset ;
	unsigned int m_frame_crop_right_offset ;
	unsigned int m_frame_crop_top_offset ;
	unsigned int m_frame_crop_bottom_offset ;
	bool m_vui_parameters_present_flag ;
	bool m_nal_hrd_parameters_present_flag ;
	bool m_vcl_hrd_parameters_present_flag ;

private:
	void skip_hrd_parameters( bit_stream_t & ) ;
	void report( const std::string & , unsigned int = 0U , const std::string & = std::string() ) ;
	void report_extra( const std::string & ) ;

private:
	std::string m_reason ;
} ;

/// \class Gr::Avc::Pps
/// A Picture Sequence Parameter Set (PPS) structure.
/// \see ISO/IEC 14496-10, esp. 7.3.2.2
/// 
class Gr::Avc::Pps 
{
public:
	Pps( const std::string & binary_pps_buffer , int sps_chroma_format_idc ) ;
		///< Constructor taking a binary byte-stuffed pps buffer.

	bool valid() const ;
		///< Returns true if a usable object.

	std::string reason() const ;
		///< Returns the in-valid() reason.

	void streamOut( std::ostream & ) const ;
		///< Streams out the pps info.

public:
	unsigned int m_nalu_type ; // 8
	unsigned int m_pic_parameter_set_id ;
	unsigned int m_seq_parameter_set_id ;
	bool m_entropy_coding_mode_flag ;
	bool m_bottom_field_pic_order_in_frame_present_flag ;
	unsigned int m_num_slice_groups_minus1 ;
	unsigned int m_num_ref_idx_10_default_active_minus1 ;
	unsigned int m_num_ref_idx_11_default_active_minus1 ;
	bool m_weighted_pred_flag ;
	unsigned int m_weighted_bipred_idc ; // 2 bits
	int m_pic_init_qp_minus26 ;
	int m_pic_init_qs_minus26 ;
	int m_chroma_qp_index_offset ;
	bool m_deblocking_filter_control_present_flag ;
	bool m_constrained_intra_pred_flag ;
	bool m_redundant_pic_cnt_present_flag ;
	bool m_transform_8x8_mode_flag ;
	bool m_pic_scaling_matrix_present_flag ;
	int m_second_chroma_qp_index_offset ;

private:
	std::string m_reason ;

private:
	void report( const std::string & ) ;
} ;

/// \class Gr::Avc::Rbsp
/// Provides static helper functions that relate to RBSP formatting.
/// 
class Gr::Avc::Rbsp
{
public:
	static void addByteStuffing( std::string & s ) ;
		///< Adds byte-stuffing to the given unstuffed RBSP buffer.

	static std::string byteStuffed( const std::string & s ) ;
		///< Returns a byte-stuffed version of the given unstuffed RBSP buffer.

	static std::string removeByteStuffing( const std::string & s ) ;
		///< Removes byte stuffing from a byte-stuffed RBSP buffer.

	static bool hasMarkers( const std::string & s ) ;
		///< Returns true if the string contains one of the inter-RBSP
		///< markers (000, 001, 002).

	static size_t findStopBit( const std::string & s , size_t fail = 0U )  ;
		///< Returns the bit offset of the RBSP stop bit.
		///< Returns the fail value on error.

	static const std::string & _000() ;
		///< Returns the 00-00-00 string.

	static const std::string & _001() ;
		///< Returns the 00-00-01 string.

	static const std::string & _002() ;
		///< Returns the 00-00-02 string.

	static const std::string & _0001() ;
		///< Returns the 00-00-00-01 string.

private:
	Rbsp() ;
} ;

#endif
