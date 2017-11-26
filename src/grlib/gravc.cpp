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
//
// gravc.cpp
//

#include "gdef.h"
#include "gravc.h"
#include "gbase64.h"
#include "gstr.h"
#include "ghexdump.h"
#include "gexpgolomb.h"
#include "gdebug.h"
#include <algorithm> // std::min()
#include <sstream>
#include <set>

namespace
{
	unsigned int fromHex( const std::string & s )
	{
		unsigned int result = 0U ;
		for( size_t i = 0U ; i < s.length() ; i++ )
		{
			result <<= 4 ;
			unsigned int c = static_cast<unsigned char>(s.at(i)) ;
			if( c >= 48U && c <= 57U ) result += c - 48U ;
			else if( c >= 65U && c <= 70U ) result += c - 65U + 10U ;
			else if( c >= 97U && c <= 102U ) result += c - 97U + 10U ;
			else throw std::runtime_error( "invalid hex string [" + s + "]" ) ;
		}
		return result ;
	}
}

Gr::Avc::Configuration Gr::Avc::Configuration::fromFmtp( const std::string & fmtp )
{
	return Configuration( /*is_fmtp=*/ true , fmtp ) ;
}

Gr::Avc::Configuration Gr::Avc::Configuration::fromAvcc( const std::string & avcc )
{
	return Configuration( /*is_fmtp=*/ false , avcc ) ;
}

Gr::Avc::Configuration::Configuration( bool is_fmtp , const std::string & s )
{
	is_fmtp ? initFmtp(s) : initAvcc(s) ;
}

Gr::Avc::Configuration::Configuration()
{
	report( "default constructed" ) ;
}

void Gr::Avc::Configuration::initFmtp( const std::string & fmtp )
{
	G::StringArray parts ; G::Str::splitIntoTokens( fmtp , parts , "; " ) ;
	for( G::StringArray::iterator p = parts.begin() ; p != parts.end() ; ++p )
	{
		typedef std::string::size_type pos_t ;
		pos_t pos = (*p).find("=") ;
		std::string key = G::Str::head( *p , pos , std::string() ) ;
		std::string value = G::Str::tail( *p , pos , std::string() ) ;
		if( key == "profile-level-id" )
		{
			// the profile and level are in the sps, but presumbly here as a separate
			// fmtp attribute for convenience -- where is the format specified?
			m_fmtp_info.profile = value.length() == 6U ? fromHex(value.substr(0U,2U)) : 999U ;
			m_fmtp_info.level = value.length() == 6U ? fromHex(value.substr(2U)) : 999U ;
			//if( value.length() != 6U ) report( "invalid profile_level in fmtp" ) ;
		}
		else if( key == "packetization-mode" ) 
		{
			m_fmtp_info.packetisation_mode = G::Str::toUInt( value ) ;
		}
		else if( key == "sprop-parameter-sets" )
		{
			G::StringArray s ; G::Str::splitIntoFields( value , s , "," ) ;
			for( G::StringArray::iterator sp = s.begin() ; sp != s.end() ; ++sp )
			{
				G_DEBUG( "Gr::Avc: fmtp parameter set [" << *sp << "]" ) ;
				std::string pset = G::Base64::decode( *sp ) ;
				G_DEBUG( "Gr::Avc: fmtp parameter set [" << G::hexdump<999>(pset) << "]" ) ;
				unsigned int nalu_type = static_cast<unsigned char>(pset.at(0U)) & 0x1f ;
				if( nalu_type == 7 ) // sps
					m_sps_list.push_back( pset ) ;
				else if( nalu_type == 8 ) // pps
					m_pps_list.push_back( pset ) ;
				else
					G_WARNING( "Gr::Avc: unexpected nalu type in fmtp: " << nalu_type ) ;
			}
		}
	}

	checkFmtp() ;
	spsListParse() ;
	ppsListParse() ;
	checkIds() ;
	commit() ;
}

void Gr::Avc::Configuration::initAvcc( const std::string & avcc_in )
{
	// the outer avcC structure is byte-oriented, but use a bit stream 
	// here for consistency with the bit-oriented sps/pps parsing
	G_DEBUG( "Gr::Avc: avcc=[" << G::hexdump<999>(avcc_in.begin(),avcc_in.end()) << "]" ) ;
	bit_stream_t stream( bit_iterator_t(avcc_in.data()) , bit_iterator_t(avcc_in.data()+avcc_in.size()) ) ;

	m_avcc_info.configuration_version = stream.get_byte() ;
	m_avcc_info.profile = stream.get_byte() ; 
	m_avcc_info.profile_compat = stream.get_byte() ;
	m_avcc_info.level = stream.get_byte() ;

	m_avcc_info.length_size_minus_one = stream.get_byte() ;
	if( ( m_avcc_info.length_size_minus_one & 0xfc ) != 0xfc ) report( "invalid reserved bits" ) ;
	m_avcc_info.length_size_minus_one &= 0x03 ;

	unsigned int sps_count = stream.get_byte() ;
	if( ( sps_count & 0xe0 ) != 0xe0 ) report( "invalid reserved bits" ) ;
	sps_count &= 0x1f ;

	unsigned int sps_size = stream.get_word() ;
	G_DEBUG( "Gr::Avc: avcc: sps_count=" << sps_count << " sps_size=" << sps_size ) ;
	for( unsigned int i = 0 ; i < sps_count ; i++ )
	{
		std::string s ;
		for( unsigned int j = 0 ; j < sps_size ; j++ )
			s.append( 1U , static_cast<char>(stream.get_byte()) ) ;
		m_sps_list.push_back( s ) ;
		G_DEBUG( "Gr::Avc: avcc: sps=[" << G::hexdump<999>(m_sps_list.back().begin(),m_sps_list.back().end()) << "]" ) ;
	}

	unsigned int pps_count = stream.get_byte() ;
	unsigned int pps_size = stream.get_word() ;
	G_DEBUG( "Gr::Avc: avcc: pps_count=" << pps_count << " pps_size=" << pps_size ) ;
	for( unsigned int i = 0 ; i < pps_count ; i++ )
	{
		std::string s ;
		for( unsigned int j = 0 ; j < pps_size ; j++ )
			s.append( 1U , static_cast<char>(stream.get_byte()) ) ;
		m_pps_list.push_back( s ) ;
		G_DEBUG( "Gr::Avc: avcc: pps=[" << G::hexdump<999>(m_pps_list.back().begin(),m_pps_list.back().end()) << "]" ) ;
	}

	checkStream( stream ) ;
	spsListParse() ;
	ppsListParse() ;
	checkIds() ;
	commit() ;
}

void Gr::Avc::Configuration::checkFmtp()
{
	if( m_fmtp_info.packetisation_mode != 1U ) 
		report( "unsuported packetisation mode in fmtp string" ) ;
}

void Gr::Avc::Configuration::checkStream( bit_stream_t & stream )
{
	if( !stream.good() )
		report( "avcc premature end" ) ;
	else if( !stream.at_end() )
		report( "avcc has trailing junk" ) ;
}

void Gr::Avc::Configuration::spsListParse()
{
	for( unsigned int i = 0 ; i < m_sps_list.size() ; i++ )
	{
		m_sps.push_back( Sps(m_sps_list.at(i)) ) ;
		if( !m_sps.back().valid() )
			report( m_sps.back().reason() ) ;
	}
}

void Gr::Avc::Configuration::ppsListParse()
{
	if( m_sps.empty() )
	{
		report( "cannot parse pps without sps" ) ;
		return ;
	}

	// arbitrarily take the chroma-format from the first sps -- should probably 
	// take account of the sps/pps interleaving order, but in practice there
	// is only one sps
	//
	int sps_chroma_format_idc = m_sps.front().m_chroma_format_idc ; 

	for( unsigned int i = 0 ; m_reason.empty() && i < m_pps_list.size() ; i++ )
	{
		m_pps.push_back( Pps(m_pps_list.at(i),sps_chroma_format_idc) ) ;
		if( !m_pps.back().valid() ) 
			report( m_pps.back().reason() ) ;
	}
}

void Gr::Avc::Configuration::checkIds()
{
	std::set<unsigned int> sps_ids ;
	for( size_t i = 0U ; i < m_sps.size() ; i++ )
	{
		unsigned int sps_id = m_sps.at(i).m_sequence_parameter_set_id ;
		if( sps_ids.find(sps_id) != sps_ids.end() ) report( "sps id not unique" ) ;
		sps_ids.insert( sps_id ) ;
	}
	std::set<unsigned int> pps_ids ;
	for( size_t i = 0U ; i < m_pps.size() ; i++ )
	{
		unsigned int pps_id = m_pps.at(i).m_pic_parameter_set_id ;
		unsigned int sps_id = m_pps.at(i).m_seq_parameter_set_id ;
		if( pps_ids.find(pps_id) != pps_ids.end() ) report( "pps id not unique" ) ;
		if( sps_ids.find(sps_id) == sps_ids.end() ) report( "invalid sps id in pps" ) ;
		pps_ids.insert( pps_id ) ;
	}
}

void Gr::Avc::Configuration::commit()
{
	if( !m_reason.empty() )
	{
		; // m_sps.clear() ; m_pps.clear() ; // moot
	}
}

void Gr::Avc::Configuration::report( const std::string & s )
{
	m_reason.append( m_reason.empty() ? s : (": "+s) ) ;
}

bool Gr::Avc::Configuration::valid() const
{
	return m_reason.empty() ;
}

std::string Gr::Avc::Configuration::reason() const
{
	return m_reason ;
}

unsigned int Gr::Avc::Configuration::nalu_length_size() const
{
	return m_avcc_info.length_size_minus_one + 1U ;
}

const std::vector<std::string> & Gr::Avc::Configuration::spsList() const
{
	return m_sps_list ;
}

const std::vector<std::string> & Gr::Avc::Configuration::ppsList() const
{
	return m_pps_list ;
}

size_t Gr::Avc::Configuration::spsCount() const
{
	return m_sps.size() ;
}

size_t Gr::Avc::Configuration::ppsCount() const
{
	return m_pps.size() ;
}

const Gr::Avc::Sps & Gr::Avc::Configuration::sps( size_t i ) const
{
	return m_sps.at(i) ;
}

const Gr::Avc::Pps & Gr::Avc::Configuration::pps( size_t i ) const
{
	return m_pps.at(i) ;
}

std::string Gr::Avc::Configuration::nalus() const
{
	// TODO preserve the SPS/PPS interleaving order
	std::string result ;
	std::string sep = Gr::Avc::Rbsp::_0001() ;
	for( size_t i = 0U ; i < m_sps_list.size() ; i++ , sep = Gr::Avc::Rbsp::_001() )
	{
		result.append( sep ) ;
		result.append( m_sps_list.at(i) ) ;
	}
	for( size_t i = 0U ; i < m_pps_list.size() ; i++ , sep = Gr::Avc::Rbsp::_001() )
	{
		result.append( sep ) ;
		result.append( m_pps_list.at(i) ) ;
	}
	return result ;
}

// ==

Gr::Avc::Sps::Sps( const std::string & sps_in )
{
	// see 14496-10 7.3.2.1.1

	std::string sps = Rbsp::removeByteStuffing(sps_in) ;
	size_t rbsp_stop_bit_pos = Rbsp::findStopBit( sps ) ;
	bit_stream_t stream( bit_iterator_t(sps.data()) , bit_iterator_t(sps.data()+sps.size()) ) ;

	m_nalu_type = stream.get_byte() & 0x1f ;
	m_profile_idc = stream.get_byte() ;
	m_constraint_set0_flag = stream.get_bool() ;
	m_constraint_set1_flag = stream.get_bool() ;
	m_constraint_set2_flag = stream.get_bool() ;
	m_constraint_set3_flag = stream.get_bool() ;
	m_constraint_set4_flag = stream.get_bool() ;
	m_constraint_set5_flag = stream.get_bool() ;
	m_zero = stream.get( 2U ) ;
	m_level_idc = stream.get_byte() ;
	m_sequence_parameter_set_id = stream.get_unsigned_golomb() ;

	bool extended_profile =
		m_profile_idc == 100 || m_profile_idc == 110 || m_profile_idc == 122 ||
		m_profile_idc == 244 || m_profile_idc == 44 || m_profile_idc == 83 ||
		m_profile_idc == 86 || m_profile_idc == 118 || m_profile_idc == 128 ;
	m_chroma_format_idc = 0 ;
	m_separate_colour_plane_flag = false ;
	m_bit_depth_luma_minus8 = 0U ;
	m_bit_depth_chroma_minus8 = 0U ;
	m_qpprime_y_zero_transform_bypass_flag = false ;
	m_seq_scaling_matrix_present_flag = false ;
	if( extended_profile )
	{
		m_chroma_format_idc = stream.get_unsigned_golomb() ;
		m_separate_colour_plane_flag = m_chroma_format_idc == 3 ? stream.get_bool() : false ;
		m_bit_depth_luma_minus8 = stream.get_unsigned_golomb() ;
		m_bit_depth_chroma_minus8 = stream.get_unsigned_golomb() ;
		m_qpprime_y_zero_transform_bypass_flag = stream.get_bool() ;
		m_seq_scaling_matrix_present_flag = stream.get_bool() ;
		if( m_seq_scaling_matrix_present_flag )
		{
			report( "sps seq_scaling_matrix_present_flag not yet implemented" ) ;
			int n = (m_chroma_format_idc != 3) ? 8 : 12 ;
			for( int i = 0 ; i < n ; i++ )
				; // TODO scaling_list()
		}
	}

	m_log2_max_frame_num_minus4 = stream.get_unsigned_golomb() ;
	m_pic_order_cnt_type = stream.get_unsigned_golomb() ;

	m_log2_max_pic_order_cnt_lsb_minus4 = 0U ;
	m_delta_pic_order_always_zero_flag = false ;
	m_offset_for_non_ref_pic = 0 ;
	m_offset_for_top_to_bottom_field = 0 ;
	m_num_ref_frames_in_pic_order_cnt_cycle = 0U ;
	if( m_pic_order_cnt_type == 0U )
	{
		m_log2_max_pic_order_cnt_lsb_minus4 = stream.get_unsigned_golomb() ;
	}
	else if( m_pic_order_cnt_type == 1U )
	{
		m_delta_pic_order_always_zero_flag = stream.get_bool() ;
		m_offset_for_non_ref_pic = stream.get_signed_golomb() ;
		m_offset_for_top_to_bottom_field = stream.get_signed_golomb() ;
		m_num_ref_frames_in_pic_order_cnt_cycle = stream.get_unsigned_golomb() ;
		for( unsigned int i = 0 ; i < m_num_ref_frames_in_pic_order_cnt_cycle ; i++ )
			(void) stream.get_signed_golomb() ;
	}

	m_max_num_ref_frames = stream.get_unsigned_golomb() ;
	m_gaps_in_frame_num_value_allowed_flag = stream.get_bool() ;
	m_pic_width_in_mbs_minus1 = stream.get_unsigned_golomb() ;
	m_pic_height_in_map_units_minus1 = stream.get_unsigned_golomb() ;

	m_frame_mbs_only_flag = stream.get_bool() ;
	m_mb_adaptive_frame_field_flag = m_frame_mbs_only_flag ? false : stream.get_bool() ; // sic

	m_direct_8x8_inference_flag = stream.get_bool() ;

	m_frame_cropping_flag = stream.get_bool() ;
	m_frame_crop_left_offset = m_frame_cropping_flag ? stream.get_unsigned_golomb() : 0U ;
	m_frame_crop_right_offset = m_frame_cropping_flag ? stream.get_unsigned_golomb() : 0U ;
	m_frame_crop_top_offset = m_frame_cropping_flag ? stream.get_unsigned_golomb() : 0U ;
	m_frame_crop_bottom_offset = m_frame_cropping_flag ? stream.get_unsigned_golomb() : 0U ;

	m_vui_parameters_present_flag = stream.get_bool() ;
	m_nal_hrd_parameters_present_flag = false ;
	m_vcl_hrd_parameters_present_flag = false ;
	if( m_vui_parameters_present_flag )
	{
		// see E.1.1
		G_DEBUG( "Gr::Avc: vui parameters preset at bit offset " << stream.tellg() ) ;
		bool aspect_ratio_info_present_flag = stream.get_bool() ;
		if( aspect_ratio_info_present_flag )
		{
			unsigned char aspect_ratio_idc = stream.get_byte() ;
			if( aspect_ratio_idc == 255U ) // Extended_SAR
			{
				/*unsigned int sar_width=*/ stream.get_word() ;
				/*unsigned int sar_height=*/ stream.get_word() ;
			}
		}
		bool overscan_info_present_flag = stream.get_bool() ;
		if( overscan_info_present_flag )
			/*unsigned char overscan_appropriate_flag=*/ stream.get_bool() ;
		bool video_signal_type_present_flag = stream.get_bool() ;
		if( video_signal_type_present_flag )
		{
			/*unsigned char video_format=*/ stream.get( 3 ) ;
			/*bool video_full_range_flag=*/ stream.get_bool() ;
			bool colour_description_present_flag = stream.get_bool() ;
			if( colour_description_present_flag )
				stream.get_byte() , stream.get_byte() , stream.get_byte() ;
		}
		bool chroma_loc_info_present_flag = stream.get_bool() ;
		if( chroma_loc_info_present_flag )
			stream.get_unsigned_golomb() , stream.get_unsigned_golomb() ;
		bool timing_info_present_flag = stream.get_bool() ;
		if( timing_info_present_flag )
			stream.get_dword() , stream.get_dword() , stream.get_bool() ;
		m_nal_hrd_parameters_present_flag = stream.get_bool() ;
		if( m_nal_hrd_parameters_present_flag )
			skip_hrd_parameters( stream ) ;
		m_vcl_hrd_parameters_present_flag = stream.get_bool() ;
		if( m_vcl_hrd_parameters_present_flag )
			skip_hrd_parameters( stream ) ;
		if( m_nal_hrd_parameters_present_flag || m_vcl_hrd_parameters_present_flag )
			stream.get_bool() ;
		/*bool pic_struct_present_flag=*/ stream.get_bool() ;
		bool stream_restriction_flag = stream.get_bool() ;
		if( stream_restriction_flag )
		{
			stream.get_bool() ;
			for( int i = 0 ; i < 6 ; i++ )
				stream.get_unsigned_golomb() ;
		}
	}
	G_DEBUG( "Gr::Avc: sps: tellg=" << stream.tellg() << " stopbit=" << rbsp_stop_bit_pos ) ;

	// report errors
	if( Rbsp::hasMarkers(sps_in) ) report( "byte stuffing error" ) ;
	if( !stream.good() ) report( "premature end of data" ) ;
	if( stream.tellg() != rbsp_stop_bit_pos ) report( "parsing did not finish at the stop bit" ) ;
	if( m_nalu_type != 7U ) report( "invalid nalu type (" , m_nalu_type , ")" ) ;
	if( m_zero != 0U ) report( "invalid zero field (" , m_zero , ")" ) ;
	if( m_pic_order_cnt_type > 1U ) report( "invalid pic_order_cnt_type (" , m_pic_order_cnt_type , ")" ) ;
	if( m_seq_scaling_matrix_present_flag ) report( "seq_scaling_matrix_present_flag not yet implemented" ) ;
	if( m_nal_hrd_parameters_present_flag ) report( "nal_hrd_parameters_present_flag not yet implemented" ) ;
	if( m_vcl_hrd_parameters_present_flag ) report( "vcl_hrd_parameters_present_flag not yet implemented" ) ;
}

unsigned int Gr::Avc::Sps::dx() const
{
	// see libav's h264.c -- willfully ignores crop_left
	unsigned int mb_width = m_pic_width_in_mbs_minus1 + 1 ;
	const bool chroma_444 = m_chroma_format_idc == 3 ;
	if( chroma_444 )
		return 16U * mb_width - std::min( m_frame_crop_right_offset , 15U ) ;
	else
		return 16U * mb_width - 2U * std::min( m_frame_crop_right_offset , 7U ) ;
}

unsigned int Gr::Avc::Sps::dy() const
{
	unsigned int mb_height = m_pic_height_in_map_units_minus1 + 1U ;
	const unsigned int chroma_y_shift = ( m_chroma_format_idc <= 1U ) ? 1U : 0U ;
	if( m_frame_mbs_only_flag )
		return 16U * mb_height - (1<<chroma_y_shift) * std::min( m_frame_crop_bottom_offset , (0x10>>chroma_y_shift)-1U ) ;
	else
		return 16U * mb_height - (2<<chroma_y_shift) * std::min( m_frame_crop_bottom_offset , (0x10>>chroma_y_shift)-1U ) ;
}

void Gr::Avc::Sps::report( const std::string & a , unsigned int b , const std::string & c )
{
	m_reason.append( m_reason.empty() ? "sps error: " : ": " ) ;
	m_reason += a ;
	if( !c.empty() )
	{
		m_reason += G::Str::fromUInt(b) ;
		m_reason += c ;
	}
}

bool Gr::Avc::Sps::valid() const
{
	return m_reason.empty() ;
}

std::string Gr::Avc::Sps::reason() const
{
	return m_reason ;
}

void Gr::Avc::Sps::skip_hrd_parameters( bit_stream_t & )
{
	; // TODO not implemented
}

void Gr::Avc::Sps::streamOut( std::ostream & stream ) const
{
	stream <<
		"profile_idc=" << m_profile_idc << " "
		"constraint_set0_flag=" << m_constraint_set0_flag << " "
		"constraint_set1_flag=" << m_constraint_set1_flag << " "
		"constraint_set2_flag=" << m_constraint_set2_flag << " "
		"constraint_set3_flag=" << m_constraint_set3_flag << " "
		"constraint_set4_flag=" << m_constraint_set4_flag << " "
		"constraint_set5_flag=" << m_constraint_set5_flag << " "
		"zero=" << m_zero << " "
		"level_idc=" << m_level_idc << " "
		"sequence_parameter_set_id=" << m_sequence_parameter_set_id << " "
		"log2_max_frame_num_minus4=" << m_log2_max_frame_num_minus4 << " "
		"pic_order_cnt_type=" << m_pic_order_cnt_type << " "
		"log2_max_pic_order_cnt_lsb_minus4=" << m_log2_max_pic_order_cnt_lsb_minus4 << " "
		"delta_pic_order_always_zero_flag=" << m_delta_pic_order_always_zero_flag << " "
		"offset_for_non_ref_pic=" << m_offset_for_non_ref_pic << " "
		"offset_for_top_to_bottom_field=" << m_offset_for_top_to_bottom_field << " "
		"num_ref_frames_in_pic_order_cnt_cycle=" << m_num_ref_frames_in_pic_order_cnt_cycle << " "
		"max_num_ref_frames=" << m_max_num_ref_frames << " "
		"gaps_in_frame_num_value_allowed_flag=" << m_gaps_in_frame_num_value_allowed_flag << " "
		"pic_width_in_mbs_minus1=" << m_pic_width_in_mbs_minus1 << " "
		"pic_height_in_map_units_minus1=" << m_pic_height_in_map_units_minus1 << " "
		"frame_mbs_only_flag=" << m_frame_mbs_only_flag << " "
		"mb_adaptive_frame_field_flag=" << m_mb_adaptive_frame_field_flag << " "
		"direct_8x8_inference_flag=" << m_direct_8x8_inference_flag << " "
		"frame_cropping_flag=" << m_frame_cropping_flag << " "
		"frame_crop_left_offset=" << m_frame_crop_left_offset << " "
		"frame_crop_right_offset=" << m_frame_crop_right_offset << " "
		"frame_crop_top_offset=" << m_frame_crop_top_offset << " "
		"frame_crop_bottom_offset=" << m_frame_crop_bottom_offset << " "
		"vui_parameters_present_flag=" << m_vui_parameters_present_flag << " "
		"dx=" << dx() << " "
		"dy=" << dy() ;
}

// ==

Gr::Avc::Pps::Pps( const std::string & pps_in , int sps_chroma_format_idc )
{
	// see 14496-10 7.3.2.2, and ff_h264_decode_picture_parameter_set() in h264_ps.c

	std::string pps = Rbsp::removeByteStuffing(pps_in) ;
	size_t rbsp_stop_bit_pos = Rbsp::findStopBit( pps ) ;
	bit_stream_t stream( bit_iterator_t(pps.data()) , bit_iterator_t(pps.data()+pps.size()) ) ;

	m_nalu_type = stream.get_byte() & 0x1f ;
	m_pic_parameter_set_id = stream.get_unsigned_golomb() ;
	m_seq_parameter_set_id = stream.get_unsigned_golomb() ;
	m_entropy_coding_mode_flag = stream.get_bool() ; // cabac vs. cavlc
	m_bottom_field_pic_order_in_frame_present_flag = stream.get_bool() ;
	m_num_slice_groups_minus1 = stream.get_unsigned_golomb() ;
	if( m_num_slice_groups_minus1 > 0U )
	{
		; // TODO slice group parsing
		report( "slice group parsing not yet implemented" ) ;
	}
	m_num_ref_idx_10_default_active_minus1 = stream.get_unsigned_golomb() ; // ref_count[0]-1
	m_num_ref_idx_11_default_active_minus1 = stream.get_unsigned_golomb() ; // ref_count[1]-1
	m_weighted_pred_flag = stream.get_bool() ;
	m_weighted_bipred_idc = stream.get(2) ;
	m_pic_init_qp_minus26 = stream.get_signed_golomb() ; // init_qp
	m_pic_init_qs_minus26 = stream.get_signed_golomb() ; // init_qs
	m_chroma_qp_index_offset = stream.get_signed_golomb() ;
	m_deblocking_filter_control_present_flag = stream.get_bool() ;
	m_constrained_intra_pred_flag = stream.get_bool() ;
	m_redundant_pic_cnt_present_flag = stream.get_bool() ;

	m_transform_8x8_mode_flag = false ;
	m_pic_scaling_matrix_present_flag = false ;
	m_second_chroma_qp_index_offset = 0 ;
	if( stream.tellg() < rbsp_stop_bit_pos ) // more_rbsp_data() ie. before rbsp_trailing_bits()
	{
		G_DEBUG( "Gr::Avc: more_rbsp_data at " << stream.tellg() ) ;
		m_transform_8x8_mode_flag = stream.get_bool() ;
		m_pic_scaling_matrix_present_flag = stream.get_bool() ;
		if( m_pic_scaling_matrix_present_flag )
		{
			if( sps_chroma_format_idc < 0 ) report( "invalid chroma format idc when parsing pps" ) ;
			int n = 6+((sps_chroma_format_idc!=3)?2:6)*(m_transform_8x8_mode_flag?1:0) ;
			G_DEBUG( "Gr::Avc: pic_scaling_matrix_present: chroma_format_idc=" << sps_chroma_format_idc << " n=" << n ) ;
			for( int i = 0 ; i < n ; i++ )
			{
				if( stream.get_bool() ) // pic_scaling_list_present_flag
				{
					; // TODO scaling_list()
					G_DEBUG( "Gr::Avc: pic_scaling_list_present but parsing not implemented" ) ;
					report( "pic_scaling_list_present_flag not yet implemented" ) ;
				}
			}
		}
		m_second_chroma_qp_index_offset = stream.get_signed_golomb() ;
	}
	G_DEBUG( "Gr::Avc: pps: tellg=" << stream.tellg() << " stopbit=" << rbsp_stop_bit_pos ) ;

	// report errors
	if( Rbsp::hasMarkers(pps_in) ) report( "byte stuffing error" ) ;
	if( !stream.good() ) report( "premature end of pps data" ) ;
	if( stream.tellg() != rbsp_stop_bit_pos ) report( "parsing did not finish at the stop bit" ) ;
	if( m_nalu_type != 8U ) report( "invalid nalu type" ) ;
}

void Gr::Avc::Pps::report( const std::string & s )
{
	m_reason.append( m_reason.empty() ? "pps error: " : ": " ) ;
	m_reason.append( s ) ;
}

bool Gr::Avc::Pps::valid() const
{
	return m_reason.empty() ;
}

std::string Gr::Avc::Pps::reason() const
{
	return m_reason ;
}

void Gr::Avc::Pps::streamOut( std::ostream & stream ) const
{
	stream <<
		"pic_parameter_set_id=" << m_pic_parameter_set_id << " "
		"seq_parameter_set_id=" << m_seq_parameter_set_id << " "
		"entropy_coding_mode_flag=" << m_entropy_coding_mode_flag << " "
		"bottom_field_pic_order_in_frame_present_flag=" << m_bottom_field_pic_order_in_frame_present_flag << " "
		"num_slice_groups_minus1=" << m_num_slice_groups_minus1 << " "
		"num_ref_idx_10_default_active_minus1=" << m_num_ref_idx_10_default_active_minus1 << " "
		"num_ref_idx_11_default_active_minus1=" << m_num_ref_idx_11_default_active_minus1 << " "
		"weighted_pred_flag=" << m_weighted_pred_flag << " "
		"weighted_bipred_idc=" << m_weighted_bipred_idc << " "
		"pic_init_qp_minus26=" << m_pic_init_qp_minus26 << " "
		"pic_init_qs_minus26=" << m_pic_init_qs_minus26 << " "
		"chroma_qp_index_offset=" << m_chroma_qp_index_offset << " "
		"deblocking_filter_control_present_flag=" << m_deblocking_filter_control_present_flag << " "
		"constrained_intra_pred_flag=" << m_constrained_intra_pred_flag << " "
		"redundant_pic_cnt_present_flag=" << m_redundant_pic_cnt_present_flag << " "
		"transform_8x8_mode_flag=" << m_transform_8x8_mode_flag << " "
		"pic_scaling_matrix_present_flag=" << m_pic_scaling_matrix_present_flag << " "
		"second_chroma_qp_index_offset=" << m_second_chroma_qp_index_offset ;
} ;

// ==

std::ostream & Gr::Avc::operator<<( std::ostream & stream , const Pps & pps )
{
	pps.streamOut( stream ) ;
	return stream ;
}

std::ostream & Gr::Avc::operator<<( std::ostream & stream , const Avc::Sps & sps )
{
	sps.streamOut( stream ) ;
	return stream ;
}

// ==

size_t Gr::Avc::Rbsp::findStopBit( const std::string & s , size_t fail ) 
{
	size_t pos = s.find_last_not_of( std::string(1U,'\0') ) ; // allow for cabac_zero_word
	if( pos == std::string::npos ) 
		return fail ; // fail if empty or all zeros

	unsigned int c = static_cast<unsigned char>(s.at(pos)) ;
	size_t result = (pos+1U) * 8U - 1U ;
	for( ; !(c&1U) ; result-- ) c >>= 1 ;
	return result ;
}

const std::string & Gr::Avc::Rbsp::_000()
{
	static const std::string s( "\x00\x00\x00" , 3U ) ;
	return s ;
}

const std::string & Gr::Avc::Rbsp::_001()
{
	static const std::string s( "\x00\x00\x01" , 3U ) ;
	return s ;
}

const std::string & Gr::Avc::Rbsp::_002()
{
	static const std::string s( "\x00\x00\x02" , 3U ) ;
	return s ;
}

const std::string & Gr::Avc::Rbsp::_0001()
{
	static const std::string s( "\x00\x00\x00\x01" , 4U ) ;
	return s ;
}

std::string Gr::Avc::Rbsp::removeByteStuffing( const std::string & s )
{
	std::string result( s ) ;
	G::Str::replaceAll( result , std::string("\x00\x00\x03",3U) , std::string("\x00\x00",2U) ) ;
	return result ;
}

void Gr::Avc::Rbsp::addByteStuffing( std::string & s )
{
	static const std::string _00( "\0\0" , 2U ) ;
	size_t pos = 0U ;
	for(;;)
	{
		pos = s.find( _00 , pos ) ;
		if( pos == std::string::npos || (pos+2U) >= s.length() )
			break ;
		char c = s.at(pos+2U) ;
		if( c == '\x00' || c == '\x01' || c == '\x02' || c == '\x03' )
		{
			s.insert( pos+2U , 1U , '\x03' ) ;
			pos += 4U ;
		}
	}
}

std::string Gr::Avc::Rbsp::byteStuffed( const std::string & s )
{
	std::string result( s ) ;
	addByteStuffing( result ) ;
	return result ;
}

bool Gr::Avc::Rbsp::hasMarkers( const std::string & s )
{
	return
		s.find(_000()) != std::string::npos ||
		s.find(_001()) != std::string::npos ||
		s.find(_002()) != std::string::npos ;
}

/// \file gravc.cpp
