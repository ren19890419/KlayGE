/**
 * @file D3D12Texture.cpp
 * @author Minmin Gong
 *
 * @section DESCRIPTION
 *
 * This source file is part of KlayGE
 * For the latest info, see http://www.klayge.org
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * You may alternatively use this source under the terms of
 * the KlayGE Proprietary License (KPL). You can obtained such a license
 * from http://www.klayge.org/licensing/.
 */

#include <KlayGE/KlayGE.hpp>
#include <KFL/Util.hpp>
#include <KFL/COMPtr.hpp>
#include <KFL/ErrorHandling.hpp>
#include <KFL/Math.hpp>
#include <KlayGE/Context.hpp>
#include <KlayGE/RenderEngine.hpp>
#include <KlayGE/RenderFactory.hpp>
#include <KlayGE/Texture.hpp>
#include <KFL/Hash.hpp>

#include <cstring>
#include <boost/assert.hpp>

#include <KlayGE/D3D12/D3D12RenderEngine.hpp>
#include <KlayGE/D3D12/D3D12Texture.hpp>

namespace KlayGE
{
	D3D12Texture::D3D12Texture(TextureType type, uint32_t sample_count, uint32_t sample_quality, uint32_t access_hint)
					: Texture(type, sample_count, sample_quality, access_hint)
	{
		if (access_hint & EAH_GPU_Write)
		{
			BOOST_ASSERT(!(access_hint & EAH_CPU_Read));
			BOOST_ASSERT(!(access_hint & EAH_CPU_Write));
		}
	}

	std::wstring const & D3D12Texture::Name() const
	{
		static const std::wstring name(L"Direct3D12 Texture");
		return name;
	}

	uint32_t D3D12Texture::Width(uint32_t level) const
	{
		KFL_UNUSED(level);
		BOOST_ASSERT(level < num_mip_maps_);

		return 1;
	}

	uint32_t D3D12Texture::Height(uint32_t level) const
	{
		KFL_UNUSED(level);
		BOOST_ASSERT(level < num_mip_maps_);

		return 1;
	}

	uint32_t D3D12Texture::Depth(uint32_t level) const
	{
		KFL_UNUSED(level);
		BOOST_ASSERT(level < num_mip_maps_);

		return 1;
	}

	void D3D12Texture::CopyToSubTexture1D(Texture& /*target*/,
			uint32_t /*dst_array_index*/, uint32_t /*dst_level*/, uint32_t /*dst_x_offset*/, uint32_t /*dst_width*/,
			uint32_t /*src_array_index*/, uint32_t /*src_level*/, uint32_t /*src_x_offset*/, uint32_t /*src_width*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::CopyToSubTexture2D(Texture& /*target*/,
			uint32_t /*dst_array_index*/, uint32_t /*dst_level*/, uint32_t /*dst_x_offset*/, uint32_t /*dst_y_offset*/, uint32_t /*dst_width*/, uint32_t /*dst_height*/,
			uint32_t /*src_array_index*/, uint32_t /*src_level*/, uint32_t /*src_x_offset*/, uint32_t /*src_y_offset*/, uint32_t /*src_width*/, uint32_t /*src_height*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::CopyToSubTexture3D(Texture& /*target*/,
			uint32_t /*dst_array_index*/, uint32_t /*dst_level*/, uint32_t /*dst_x_offset*/, uint32_t /*dst_y_offset*/, uint32_t /*dst_z_offset*/, uint32_t /*dst_width*/, uint32_t /*dst_height*/, uint32_t /*dst_depth*/,
			uint32_t /*src_array_index*/, uint32_t /*src_level*/, uint32_t /*src_x_offset*/, uint32_t /*src_y_offset*/, uint32_t /*src_z_offset*/, uint32_t /*src_width*/, uint32_t /*src_height*/, uint32_t /*src_depth*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::CopyToSubTextureCube(Texture& /*target*/,
			uint32_t /*dst_array_index*/, CubeFaces /*dst_face*/, uint32_t /*dst_level*/, uint32_t /*dst_x_offset*/, uint32_t /*dst_y_offset*/, uint32_t /*dst_width*/, uint32_t /*dst_height*/,
			uint32_t /*src_array_index*/, CubeFaces /*src_face*/, uint32_t /*src_level*/, uint32_t /*src_x_offset*/, uint32_t /*src_y_offset*/, uint32_t /*src_width*/, uint32_t /*src_height*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	D3D12ShaderResourceViewSimulationPtr const & D3D12Texture::RetriveD3DShaderResourceView(uint32_t first_array_index, uint32_t num_items,
		uint32_t first_level, uint32_t num_levels)
	{
		BOOST_ASSERT(this->AccessHint() & EAH_GPU_Read);

		if (this->HWResourceReady())
		{
			size_t hash_val = HashValue(first_array_index);
			HashCombine(hash_val, num_items);
			HashCombine(hash_val, first_level);
			HashCombine(hash_val, num_levels);

			auto iter = d3d_sr_views_.find(hash_val);
			if (iter != d3d_sr_views_.end())
			{
				return iter->second;
			}
			else
			{
				auto desc = this->FillSRVDesc(first_array_index, num_items, first_level, num_levels);
				D3D12ShaderResourceViewSimulationPtr sr_view = MakeSharedPtr<D3D12ShaderResourceViewSimulation>(this, desc);
				return d3d_sr_views_.emplace(hash_val, sr_view).first->second;
			}
		}
		else
		{
			static D3D12ShaderResourceViewSimulationPtr const view;
			return view;
		}
	}

	D3D12UnorderedAccessViewSimulationPtr const & D3D12Texture::RetriveD3DUnorderedAccessView(uint32_t first_array_index, uint32_t num_items,
		uint32_t level)
	{
		BOOST_ASSERT(this->AccessHint() & EAH_GPU_Unordered);

		if (this->HWResourceReady())
		{
			size_t hash_val = HashValue(first_array_index);
			HashCombine(hash_val, num_items);
			HashCombine(hash_val, level);
			HashCombine(hash_val, 0);
			HashCombine(hash_val, 0);

			auto iter = d3d_ua_views_.find(hash_val);
			if (iter != d3d_ua_views_.end())
			{
				return iter->second;
			}
			else
			{
				auto desc = this->FillUAVDesc(first_array_index, num_items, level);
				D3D12UnorderedAccessViewSimulationPtr ua_view = MakeSharedPtr<D3D12UnorderedAccessViewSimulation>(this, desc);
				return d3d_ua_views_.emplace(hash_val, ua_view).first->second;
			}
		}
		else
		{
			static D3D12UnorderedAccessViewSimulationPtr const view;
			return view;
		}
	}

	D3D12UnorderedAccessViewSimulationPtr const & D3D12Texture::RetriveD3DUnorderedAccessView(uint32_t array_index, uint32_t first_slice,
		uint32_t num_slices, uint32_t level)
	{
		BOOST_ASSERT(this->AccessHint() & EAH_GPU_Unordered);

		if (this->HWResourceReady())
		{
			size_t hash_val = HashValue(array_index);
			HashCombine(hash_val, 1);
			HashCombine(hash_val, level);
			HashCombine(hash_val, first_slice);
			HashCombine(hash_val, num_slices);

			auto iter = d3d_ua_views_.find(hash_val);
			if (iter != d3d_ua_views_.end())
			{
				return iter->second;
			}
			else
			{
				auto desc = this->FillUAVDesc(array_index, first_slice, num_slices, level);
				D3D12UnorderedAccessViewSimulationPtr ua_view = MakeSharedPtr<D3D12UnorderedAccessViewSimulation>(this, desc);
				return d3d_ua_views_.emplace(hash_val, ua_view).first->second;
			}
		}
		else
		{
			static D3D12UnorderedAccessViewSimulationPtr const view;
			return view;
		}
	}

	D3D12UnorderedAccessViewSimulationPtr const & D3D12Texture::RetriveD3DUnorderedAccessView(uint32_t first_array_index, uint32_t num_items,
		CubeFaces first_face, uint32_t num_faces, uint32_t level)
	{
		BOOST_ASSERT(this->AccessHint() & EAH_GPU_Unordered);

		if (this->HWResourceReady())
		{
			size_t hash_val = HashValue(first_array_index * 6 + first_face);
			HashCombine(hash_val, num_items * 6 + num_faces);
			HashCombine(hash_val, level);
			HashCombine(hash_val, 0);
			HashCombine(hash_val, 0);

			auto iter = d3d_ua_views_.find(hash_val);
			if (iter != d3d_ua_views_.end())
			{
				return iter->second;
			}
			else
			{
				auto desc = this->FillUAVDesc(first_array_index, num_items, first_face, num_faces, level);
				D3D12UnorderedAccessViewSimulationPtr ua_view = MakeSharedPtr<D3D12UnorderedAccessViewSimulation>(this, desc);
				return d3d_ua_views_.emplace(hash_val, ua_view).first->second;
			}
		}
		else
		{
			static D3D12UnorderedAccessViewSimulationPtr const view;
			return view;
		}
	}

	D3D12RenderTargetViewSimulationPtr const & D3D12Texture::RetriveD3DRenderTargetView(uint32_t first_array_index, uint32_t array_size,
		uint32_t level)
	{
		BOOST_ASSERT(this->AccessHint() & EAH_GPU_Write);
		BOOST_ASSERT(first_array_index < this->ArraySize());
		BOOST_ASSERT(first_array_index + array_size <= this->ArraySize());

		if (this->HWResourceReady())
		{
			size_t hash_val = HashValue(first_array_index);
			HashCombine(hash_val, array_size);
			HashCombine(hash_val, level);
			HashCombine(hash_val, 0);
			HashCombine(hash_val, 0);

			auto iter = d3d_rt_views_.find(hash_val);
			if (iter != d3d_rt_views_.end())
			{
				return iter->second;
			}
			else
			{
				auto desc = this->FillRTVDesc(first_array_index, array_size, level);
				D3D12RenderTargetViewSimulationPtr rt_view = MakeSharedPtr<D3D12RenderTargetViewSimulation>(this, desc);
				return d3d_rt_views_.emplace(hash_val, rt_view).first->second;
			}
		}
		else
		{
			static D3D12RenderTargetViewSimulationPtr const view;
			return view;
		}
	}

	D3D12RenderTargetViewSimulationPtr const & D3D12Texture::RetriveD3DRenderTargetView(uint32_t array_index, uint32_t first_slice,
		uint32_t num_slices, uint32_t level)
	{
		BOOST_ASSERT(this->AccessHint() & EAH_GPU_Write);
		BOOST_ASSERT(0 == array_index);

		if (this->HWResourceReady())
		{
			size_t hash_val = HashValue(array_index);
			HashCombine(hash_val, 1);
			HashCombine(hash_val, level);
			HashCombine(hash_val, first_slice);
			HashCombine(hash_val, num_slices);

			auto iter = d3d_rt_views_.find(hash_val);
			if (iter != d3d_rt_views_.end())
			{
				return iter->second;
			}
			else
			{
				auto desc = this->FillRTVDesc(array_index, first_slice, num_slices, level);
				D3D12RenderTargetViewSimulationPtr rt_view = MakeSharedPtr<D3D12RenderTargetViewSimulation>(this, desc);
				return d3d_rt_views_.emplace(hash_val, rt_view).first->second;
			}
		}
		else
		{
			static D3D12RenderTargetViewSimulationPtr const view;
			return view;
		}
	}

	D3D12RenderTargetViewSimulationPtr const & D3D12Texture::RetriveD3DRenderTargetView(uint32_t array_index, CubeFaces face,
		uint32_t level)
	{
		BOOST_ASSERT(this->AccessHint() & EAH_GPU_Write);

		if (this->HWResourceReady())
		{
			size_t hash_val = HashValue(array_index * 6 + face);
			HashCombine(hash_val, 1);
			HashCombine(hash_val, level);
			HashCombine(hash_val, 0);
			HashCombine(hash_val, 0);

			auto iter = d3d_rt_views_.find(hash_val);
			if (iter != d3d_rt_views_.end())
			{
				return iter->second;
			}
			else
			{
				auto desc = this->FillRTVDesc(array_index, face, level);
				D3D12RenderTargetViewSimulationPtr rt_view = MakeSharedPtr<D3D12RenderTargetViewSimulation>(this, desc);
				return d3d_rt_views_.emplace(hash_val, rt_view).first->second;
			}
		}
		else
		{
			static D3D12RenderTargetViewSimulationPtr const view;
			return view;
		}
	}

	D3D12DepthStencilViewSimulationPtr const & D3D12Texture::RetriveD3DDepthStencilView(uint32_t first_array_index, uint32_t array_size,
		uint32_t level)
	{
		BOOST_ASSERT(this->AccessHint() & EAH_GPU_Write);
		BOOST_ASSERT(first_array_index < this->ArraySize());
		BOOST_ASSERT(first_array_index + array_size <= this->ArraySize());

		if (this->HWResourceReady())
		{
			size_t hash_val = HashValue(first_array_index);
			HashCombine(hash_val, array_size);
			HashCombine(hash_val, level);
			HashCombine(hash_val, 0);
			HashCombine(hash_val, 0);

			auto iter = d3d_ds_views_.find(hash_val);
			if (iter != d3d_ds_views_.end())
			{
				return iter->second;
			}
			else
			{
				auto desc = this->FillDSVDesc(first_array_index, array_size, level);
				D3D12DepthStencilViewSimulationPtr ds_view = MakeSharedPtr<D3D12DepthStencilViewSimulation>(this, desc);
				return d3d_ds_views_.emplace(hash_val, ds_view).first->second;
			}
		}
		else
		{
			static D3D12DepthStencilViewSimulationPtr const view;
			return view;
		}
	}

	D3D12DepthStencilViewSimulationPtr const & D3D12Texture::RetriveD3DDepthStencilView(uint32_t array_index, uint32_t first_slice, uint32_t num_slices,
		uint32_t level)
	{
		BOOST_ASSERT(this->AccessHint() & EAH_GPU_Write);
		BOOST_ASSERT(0 == array_index);

		if (this->HWResourceReady())
		{
			size_t hash_val = HashValue(array_index);
			HashCombine(hash_val, 1);
			HashCombine(hash_val, level);
			HashCombine(hash_val, first_slice);
			HashCombine(hash_val, num_slices);

			auto iter = d3d_ds_views_.find(hash_val);
			if (iter != d3d_ds_views_.end())
			{
				return iter->second;
			}
			else
			{
				auto desc = this->FillDSVDesc(array_index, first_slice, num_slices, level);
				D3D12DepthStencilViewSimulationPtr ds_view = MakeSharedPtr<D3D12DepthStencilViewSimulation>(this, desc);
				return d3d_ds_views_.emplace(hash_val, ds_view).first->second;
			}
		}
		else
		{
			static D3D12DepthStencilViewSimulationPtr const view;
			return view;
		}
	}

	D3D12DepthStencilViewSimulationPtr const & D3D12Texture::RetriveD3DDepthStencilView(uint32_t array_index, CubeFaces face, uint32_t level)
	{
		BOOST_ASSERT(this->AccessHint() & EAH_GPU_Write);

		if (this->HWResourceReady())
		{
			size_t hash_val = HashValue(array_index * 6 + face);
			HashCombine(hash_val, 1);
			HashCombine(hash_val, level);
			HashCombine(hash_val, 0);
			HashCombine(hash_val, 0);

			auto iter = d3d_ds_views_.find(hash_val);
			if (iter != d3d_ds_views_.end())
			{
				return iter->second;
			}
			else
			{
				auto desc = this->FillDSVDesc(array_index, face, level);
				D3D12DepthStencilViewSimulationPtr ds_view = MakeSharedPtr<D3D12DepthStencilViewSimulation>(this, desc);
				return d3d_ds_views_.emplace(hash_val, ds_view).first->second;
			}
		}
		else
		{
			static D3D12DepthStencilViewSimulationPtr const view;
			return view;
		}
	}

	void D3D12Texture::Map1D(uint32_t /*array_index*/, uint32_t /*level*/, TextureMapAccess /*tma*/,
			uint32_t /*x_offset*/, uint32_t /*width*/,
			void*& /*data*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::Map2D(uint32_t /*array_index*/, uint32_t /*level*/, TextureMapAccess /*tma*/,
			uint32_t /*x_offset*/, uint32_t /*y_offset*/, uint32_t /*width*/, uint32_t /*height*/,
			void*& /*data*/, uint32_t& /*row_pitch*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::Map3D(uint32_t /*array_index*/, uint32_t /*level*/, TextureMapAccess /*tma*/,
			uint32_t /*x_offset*/, uint32_t /*y_offset*/, uint32_t /*z_offset*/,
			uint32_t /*width*/, uint32_t /*height*/, uint32_t /*depth*/,
			void*& /*data*/, uint32_t& /*row_pitch*/, uint32_t& /*slice_pitch*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::MapCube(uint32_t /*array_index*/, CubeFaces /*face*/, uint32_t /*level*/, TextureMapAccess /*tma*/,
			uint32_t /*x_offset*/, uint32_t /*y_offset*/, uint32_t /*width*/, uint32_t /*height*/,
			void*& /*data*/, uint32_t& /*row_pitch*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::Unmap1D(uint32_t /*array_index*/, uint32_t /*level*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::Unmap2D(uint32_t /*array_index*/, uint32_t /*level*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::Unmap3D(uint32_t /*array_index*/, uint32_t /*level*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::UnmapCube(uint32_t /*array_index*/, CubeFaces /*face*/, uint32_t /*level*/)
	{
		KFL_UNREACHABLE("Can't be called");
	}

	void D3D12Texture::DeleteHWResource()
	{
		d3d_resource_.reset();
		d3d_texture_upload_heaps_.reset();
		d3d_texture_readback_heaps_.reset();
	}

	bool D3D12Texture::HWResourceReady() const
	{
		return d3d_resource_.get() ? true : false;
	}

	void D3D12Texture::DoHWCopyToTexture(Texture& target)
	{
		D3D12RenderEngine& re = *checked_cast<D3D12RenderEngine*>(&Context::Instance().RenderFactoryInstance().RenderEngineInstance());
		ID3D12GraphicsCommandList* cmd_list = re.D3DRenderCmdList();

		if ((access_hint_ & EAH_CPU_Write) && (target.AccessHint() & EAH_GPU_Read))
		{
			re.ForceCPUGPUSync();
		}

		D3D12Texture& other = *checked_cast<D3D12Texture*>(&target);

		uint32_t const num_subres = array_size_ * num_mip_maps_;

		UINT n = 0;
		D3D12_RESOURCE_BARRIER barriers[2];
		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		if (this->UpdateResourceBarrier(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, barrier, D3D12_RESOURCE_STATE_COPY_SOURCE))
		{
			barriers[n] = barrier;
			++ n;
		}
		if (other.UpdateResourceBarrier(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, barrier, D3D12_RESOURCE_STATE_COPY_DEST))
		{
			barriers[n] = barrier;
			++ n;
		}
		if (n > 0)
		{
			cmd_list->ResourceBarrier(n, barriers);
		}

		if ((this->SampleCount() > 1) && (1 == target.SampleCount()))
		{
			for (uint32_t i = 0; i < num_subres; ++ i)
			{
				cmd_list->ResolveSubresource(other.D3DResource().get(), i, d3d_resource_.get(), i, dxgi_fmt_);
			}
		}
		else
		{
			cmd_list->CopyResource(other.D3DResource().get(), d3d_resource_.get());
		}
	}

	void D3D12Texture::DoHWCopyToSubTexture(Texture& target,
			uint32_t dst_subres, uint32_t dst_x_offset, uint32_t dst_y_offset, uint32_t dst_z_offset,
			uint32_t src_subres, uint32_t src_x_offset, uint32_t src_y_offset, uint32_t src_z_offset,
			uint32_t width, uint32_t height, uint32_t depth)
	{
		D3D12RenderEngine& re = *checked_cast<D3D12RenderEngine*>(&Context::Instance().RenderFactoryInstance().RenderEngineInstance());
		ID3D12GraphicsCommandList* cmd_list = re.D3DRenderCmdList();

		if ((access_hint_ & EAH_CPU_Write) && (target.AccessHint() & EAH_GPU_Read))
		{
			re.ForceCPUGPUSync();
		}

		D3D12Texture& other = *checked_cast<D3D12Texture2D*>(&target);

		UINT n = 0;
		D3D12_RESOURCE_BARRIER barriers[2];
		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		if (this->UpdateResourceBarrier(src_subres, barrier, D3D12_RESOURCE_STATE_COPY_SOURCE))
		{
			barriers[n] = barrier;
			++ n;
		}
		if (other.UpdateResourceBarrier(dst_subres, barrier, D3D12_RESOURCE_STATE_COPY_DEST))
		{
			barriers[n] = barrier;
			++ n;
		}
		if (n > 0)
		{
			cmd_list->ResourceBarrier(n, barriers);
		}

		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = d3d_resource_.get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = src_subres;

		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.pResource = other.D3DResource().get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = dst_subres;

		D3D12_BOX src_box;
		src_box.left = src_x_offset;
		src_box.top = src_y_offset;
		src_box.front = src_z_offset;
		src_box.right = src_x_offset + width;
		src_box.bottom = src_y_offset + height;
		src_box.back = src_z_offset + depth;

		cmd_list->CopyTextureRegion(&dst, dst_x_offset, dst_y_offset, dst_z_offset, &src, &src_box);
	}

	void D3D12Texture::DoCreateHWResource(D3D12_RESOURCE_DIMENSION dim,
			uint32_t width, uint32_t height, uint32_t depth, uint32_t array_size,
			ArrayRef<ElementInitData> init_data, float4 const * clear_value_hint)
	{
		D3D12RenderEngine& re = *checked_cast<D3D12RenderEngine*>(&Context::Instance().RenderFactoryInstance().RenderEngineInstance());
		ID3D12Device* device = re.D3DDevice();

		D3D12_CLEAR_VALUE clear_value;

		D3D12_RESOURCE_DESC tex_desc;
		tex_desc.Dimension = dim;
		tex_desc.Alignment = 0;
		tex_desc.Width = width;
		tex_desc.Height = height;
		switch (dim)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			tex_desc.DepthOrArraySize = static_cast<UINT16>(array_size);
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			tex_desc.DepthOrArraySize = static_cast<UINT16>(depth);
			break;

		default:
			KFL_UNREACHABLE("Invalid resource dimension");
		}
		tex_desc.MipLevels = static_cast<UINT16>(num_mip_maps_);
		tex_desc.Format = dxgi_fmt_;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		tex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (access_hint_ & EAH_GPU_Write)
		{
			if (IsDepthFormat(format_))
			{
				tex_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

				switch (format_)
				{
				case EF_D16:
					clear_value.Format = DXGI_FORMAT_D16_UNORM;
					break;

				case EF_D24S8:
					clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
					break;

				case EF_D32F:
					clear_value.Format = DXGI_FORMAT_D32_FLOAT;
					break;

				default:
					KFL_UNREACHABLE("Invalid depth format");
				}
				if (clear_value_hint != nullptr)
				{
					clear_value.DepthStencil.Depth = (*clear_value_hint)[0];
					clear_value.DepthStencil.Stencil = static_cast<UINT8>((*clear_value_hint)[1] + 0.5f);
				}
				else
				{
					clear_value.DepthStencil.Depth = 1.0f;
					clear_value.DepthStencil.Stencil = 0;
				}
			}
			else
			{
				tex_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

				clear_value.Format = dxgi_fmt_;
				if (clear_value_hint != nullptr)
				{
					clear_value.Color[0] = (*clear_value_hint)[0];
					clear_value.Color[1] = (*clear_value_hint)[1];
					clear_value.Color[2] = (*clear_value_hint)[2];
					clear_value.Color[3] = (*clear_value_hint)[3];
				}
				else
				{
					clear_value.Color[0] = clear_value.Color[1] = clear_value.Color[2] = clear_value.Color[3] = 0;
				}
			}
		}
		if (access_hint_ & EAH_GPU_Unordered)
		{
			tex_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		D3D12_HEAP_PROPERTIES heap_prop;
		heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
		heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap_prop.CreationNodeMask = 0;
		heap_prop.VisibleNodeMask = 0;

		D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_COMMON;
		if (IsDepthFormat(format_) && (access_hint_ & EAH_GPU_Write))
		{
			init_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			std::fill(curr_states_.begin(), curr_states_.end(), init_state);
		}

		ID3D12Resource* d3d_texture;
		TIFHR(device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE,
			&tex_desc, init_state, (access_hint_ & EAH_GPU_Write) ? &clear_value : nullptr,
			IID_ID3D12Resource, reinterpret_cast<void**>(&d3d_texture)));
		d3d_resource_ = MakeCOMPtr(d3d_texture);

		uint32_t const num_subres = array_size * num_mip_maps_;
		uint64_t upload_buffer_size = 0;
		device->GetCopyableFootprints(&tex_desc, 0, num_subres, 0, nullptr, nullptr, nullptr, &upload_buffer_size);

		D3D12_HEAP_PROPERTIES upload_heap_prop;
		upload_heap_prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		upload_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		upload_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		upload_heap_prop.CreationNodeMask = 0;
		upload_heap_prop.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC buff_desc;
		buff_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		buff_desc.Alignment = 0;
		buff_desc.Width = upload_buffer_size;
		buff_desc.Height = 1;
		buff_desc.DepthOrArraySize = 1;
		buff_desc.MipLevels = 1;
		buff_desc.Format = DXGI_FORMAT_UNKNOWN;
		buff_desc.SampleDesc.Count = 1;
		buff_desc.SampleDesc.Quality = 0;
		buff_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		buff_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ID3D12Resource* d3d_texture_upload_heaps;
		TIFHR(device->CreateCommittedResource(&upload_heap_prop, D3D12_HEAP_FLAG_NONE, &buff_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_ID3D12Resource, reinterpret_cast<void**>(&d3d_texture_upload_heaps)));
		d3d_texture_upload_heaps_ = MakeCOMPtr(d3d_texture_upload_heaps);

		D3D12_HEAP_PROPERTIES readback_heap_prop;
		readback_heap_prop.Type = D3D12_HEAP_TYPE_READBACK;
		readback_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		readback_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		readback_heap_prop.CreationNodeMask = 0;
		readback_heap_prop.VisibleNodeMask = 0;

		ID3D12Resource* d3d_texture_readback_heaps;
		TIFHR(device->CreateCommittedResource(&readback_heap_prop, D3D12_HEAP_FLAG_NONE, &buff_desc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			IID_ID3D12Resource, reinterpret_cast<void**>(&d3d_texture_readback_heaps)));
		d3d_texture_readback_heaps_ = MakeCOMPtr(d3d_texture_readback_heaps);

		if (!init_data.empty())
		{
			ID3D12GraphicsCommandList* cmd_list = re.D3DResCmdList();
			std::lock_guard<std::mutex> lock(re.D3DResCmdListMutex());

			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			if (this->UpdateResourceBarrier(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, barrier, D3D12_RESOURCE_STATE_COPY_DEST))
			{
				cmd_list->ResourceBarrier(1, &barrier);
			}

			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(num_subres);
			std::vector<uint64_t> row_sizes_in_bytes(num_subres);
			std::vector<uint32_t> num_rows(num_subres);

			uint64_t required_size = 0;
			device->GetCopyableFootprints(&tex_desc, 0, num_subres, 0, &layouts[0], &num_rows[0], &row_sizes_in_bytes[0], &required_size);

			uint8_t* p;
			d3d_texture_upload_heaps_->Map(0, nullptr, reinterpret_cast<void**>(&p));
			for (uint32_t i = 0; i < num_subres; ++ i)
			{
				D3D12_SUBRESOURCE_DATA src_data;
				src_data.pData = init_data[i].data;
				src_data.RowPitch = init_data[i].row_pitch;
				src_data.SlicePitch = init_data[i].slice_pitch;

				D3D12_MEMCPY_DEST dest_data;
				dest_data.pData = p + layouts[i].Offset;
				dest_data.RowPitch = layouts[i].Footprint.RowPitch;
				dest_data.SlicePitch = layouts[i].Footprint.RowPitch * num_rows[i];

				for (UINT z = 0; z < layouts[i].Footprint.Depth; ++ z)
				{
					uint8_t const * src_slice
						= reinterpret_cast<uint8_t const *>(src_data.pData) + src_data.SlicePitch * z;
					uint8_t* dest_slice = reinterpret_cast<uint8_t*>(dest_data.pData) + dest_data.SlicePitch * z;
					for (UINT y = 0; y < num_rows[i]; ++ y)
					{
						memcpy(dest_slice + dest_data.RowPitch * y, src_slice + src_data.RowPitch * y,
							static_cast<size_t>(row_sizes_in_bytes[i]));
					}
				}
			}
			d3d_texture_upload_heaps_->Unmap(0, nullptr);

			for (uint32_t i = 0; i < num_subres; ++ i)
			{
				D3D12_TEXTURE_COPY_LOCATION src;
				src.pResource = d3d_texture_upload_heaps_.get();
				src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				src.PlacedFootprint = layouts[i];

				D3D12_TEXTURE_COPY_LOCATION dst;
				dst.pResource = d3d_resource_.get();
				dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				dst.SubresourceIndex = i;

				cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
			}

			re.CommitResCmd();
		}
	}

	void D3D12Texture::DoMap(uint32_t subres, TextureMapAccess tma,
			uint32_t x_offset, uint32_t y_offset, uint32_t z_offset,
			uint32_t width, uint32_t height, uint32_t depth,
			void*& data, uint32_t& row_pitch, uint32_t& slice_pitch)
	{
		D3D12RenderEngine& re = *checked_cast<D3D12RenderEngine*>(&Context::Instance().RenderFactoryInstance().RenderEngineInstance());
		ID3D12Device* device = re.D3DDevice();

		KFL_UNUSED(width);
		KFL_UNUSED(height);
		KFL_UNUSED(depth);

		last_tma_ = tma;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		D3D12_RESOURCE_DESC const tex_desc = d3d_resource_->GetDesc();
		uint64_t required_size = 0;
		device->GetCopyableFootprints(&tex_desc, subres, 1, 0, &layout, nullptr, nullptr, &required_size);

		if ((TMA_Read_Only == tma) || (TMA_Read_Write == tma))
		{
			ID3D12GraphicsCommandList* cmd_list = re.D3DRenderCmdList();

			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			if (this->UpdateResourceBarrier(subres, barrier, D3D12_RESOURCE_STATE_COPY_SOURCE))
			{
				cmd_list->ResourceBarrier(1, &barrier);
			}

			D3D12_TEXTURE_COPY_LOCATION src;
			src.pResource = d3d_resource_.get();
			src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			src.SubresourceIndex = subres;

			D3D12_TEXTURE_COPY_LOCATION dst;
			dst.pResource = d3d_texture_readback_heaps_.get();
			dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			dst.PlacedFootprint = layout;

			cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

			re.ForceCPUGPUSync();
		}

		uint8_t* p;
		d3d_texture_upload_heaps_->Map(0, nullptr, reinterpret_cast<void**>(&p));

		data = p + layout.Offset + (z_offset * layout.Footprint.Height + y_offset) * layout.Footprint.RowPitch
			+ x_offset * NumFormatBytes(format_);
		row_pitch = layout.Footprint.RowPitch;
		slice_pitch = layout.Footprint.RowPitch * layout.Footprint.Height;

		if ((TMA_Read_Only == tma) || (TMA_Read_Write == tma))
		{
			d3d_texture_readback_heaps_->Map(0, nullptr, reinterpret_cast<void**>(&p));
			uint8_t* src_p = p + layout.Offset + (z_offset * layout.Footprint.Height + y_offset) * layout.Footprint.RowPitch
				+ x_offset * NumFormatBytes(format_);
			uint8_t* dst_p = static_cast<uint8_t*>(data);
			for (uint32_t z = 0; z < depth; ++ z)
			{
				memcpy(dst_p + z * slice_pitch, src_p + z * slice_pitch, row_pitch * height);
			}
			d3d_texture_readback_heaps_->Unmap(0, nullptr);
		}
	}

	void D3D12Texture::DoUnmap(uint32_t subres)
	{
		D3D12RenderEngine& re = *checked_cast<D3D12RenderEngine*>(&Context::Instance().RenderFactoryInstance().RenderEngineInstance());
		ID3D12Device* device = re.D3DDevice();

		d3d_texture_upload_heaps_->Unmap(0, nullptr);

		if ((TMA_Write_Only == last_tma_) || (TMA_Read_Write == last_tma_))
		{
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
			D3D12_RESOURCE_DESC const tex_desc = d3d_resource_->GetDesc();
			uint64_t required_size = 0;
			device->GetCopyableFootprints(&tex_desc, subres, 1, 0, &layout, nullptr, nullptr, &required_size);

			ID3D12GraphicsCommandList* cmd_list = re.D3DRenderCmdList();

			re.ForceCPUGPUSync();

			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			if (this->UpdateResourceBarrier(subres, barrier, D3D12_RESOURCE_STATE_COPY_DEST))
			{
				cmd_list->ResourceBarrier(1, &barrier);
			}

			D3D12_TEXTURE_COPY_LOCATION src;
			src.pResource = d3d_texture_upload_heaps_.get();
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = layout;

			D3D12_TEXTURE_COPY_LOCATION dst;
			dst.pResource = d3d_resource_.get();
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = subres;

			cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC D3D12Texture::FillUAVDesc(uint32_t first_array_index, uint32_t num_items, uint32_t level) const
	{
		KFL_UNUSED(first_array_index);
		KFL_UNUSED(num_items);
		KFL_UNUSED(level);

		KFL_UNREACHABLE("Can't be called");
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC D3D12Texture::FillUAVDesc(uint32_t array_index, uint32_t first_slice, uint32_t num_slices,
		uint32_t level) const
	{
		KFL_UNUSED(array_index);
		KFL_UNUSED(first_slice);
		KFL_UNUSED(num_slices);
		KFL_UNUSED(level);

		KFL_UNREACHABLE("Can't be called");
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC D3D12Texture::FillUAVDesc(uint32_t first_array_index, uint32_t num_items,
		CubeFaces first_face, uint32_t num_faces, uint32_t level) const
	{
		KFL_UNUSED(first_array_index);
		KFL_UNUSED(num_items);
		KFL_UNUSED(first_face);
		KFL_UNUSED(num_faces);
		KFL_UNUSED(level);

		KFL_UNREACHABLE("Can't be called");
	}

	D3D12_RENDER_TARGET_VIEW_DESC D3D12Texture::FillRTVDesc(uint32_t first_array_index, uint32_t array_size, uint32_t level) const
	{
		KFL_UNUSED(first_array_index);
		KFL_UNUSED(array_size);
		KFL_UNUSED(level);

		KFL_UNREACHABLE("Can't be called");
	}

	D3D12_RENDER_TARGET_VIEW_DESC D3D12Texture::FillRTVDesc(uint32_t array_index, uint32_t first_slice, uint32_t num_slices,
		uint32_t level) const
	{
		KFL_UNUSED(array_index);
		KFL_UNUSED(first_slice);
		KFL_UNUSED(num_slices);
		KFL_UNUSED(level);

		KFL_UNREACHABLE("Can't be called");
	}

	D3D12_RENDER_TARGET_VIEW_DESC D3D12Texture::FillRTVDesc(uint32_t array_index, CubeFaces face, uint32_t level) const
	{
		KFL_UNUSED(array_index);
		KFL_UNUSED(face);
		KFL_UNUSED(level);

		KFL_UNREACHABLE("Can't be called");
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC D3D12Texture::FillDSVDesc(uint32_t first_array_index, uint32_t array_size, uint32_t level) const
	{
		KFL_UNUSED(first_array_index);
		KFL_UNUSED(array_size);
		KFL_UNUSED(level);

		KFL_UNREACHABLE("Can't be called");
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC D3D12Texture::FillDSVDesc(uint32_t array_index, uint32_t first_slice, uint32_t num_slices,
		uint32_t level) const
	{
		KFL_UNUSED(array_index);
		KFL_UNUSED(first_slice);
		KFL_UNUSED(num_slices);
		KFL_UNUSED(level);

		KFL_UNREACHABLE("Can't be called");
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC D3D12Texture::FillDSVDesc(uint32_t array_index, CubeFaces face, uint32_t level) const
	{
		KFL_UNUSED(array_index);
		KFL_UNUSED(face);
		KFL_UNUSED(level);

		KFL_UNREACHABLE("Can't be called");
	}

	// TODO: Figure out how to reuse Map/Unmap for UpdateSubresource
	
	void D3D12Texture::UpdateSubresource1D(uint32_t array_index, uint32_t level,
		uint32_t x_offset, uint32_t width,
		void const * data)
	{
		D3D12RenderEngine& re = *checked_cast<D3D12RenderEngine*>(&Context::Instance().RenderFactoryInstance().RenderEngineInstance());
		ID3D12Device* device = re.D3DDevice();

		D3D12_RESOURCE_DESC tex_desc;
		tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		tex_desc.Alignment = 0;
		tex_desc.Width = width;
		tex_desc.Height = 1;
		tex_desc.DepthOrArraySize = 1;
		tex_desc.MipLevels = 1;
		tex_desc.Format = dxgi_fmt_;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		tex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		uint64_t row_sizes_in_byte;
		uint32_t num_row;
		uint64_t upload_buffer_size = 0;
		device->GetCopyableFootprints(&tex_desc, 0, 1, 0, &layout, &num_row, &row_sizes_in_byte, &upload_buffer_size);

		{
			uint8_t* p;
			d3d_texture_upload_heaps_->Map(0, nullptr, reinterpret_cast<void**>(&p));

			D3D12_SUBRESOURCE_DATA src_data;
			src_data.pData = data;
			src_data.RowPitch = width * NumFormatBytes(format_);
			src_data.SlicePitch = src_data.RowPitch;

			D3D12_MEMCPY_DEST dest_data;
			dest_data.pData = p + layout.Offset;
			dest_data.RowPitch = layout.Footprint.RowPitch;
			dest_data.SlicePitch = layout.Footprint.RowPitch * num_row;

			{
				uint8_t const * src_slice
					= reinterpret_cast<uint8_t const *>(src_data.pData);
				uint8_t* dest_slice = reinterpret_cast<uint8_t*>(dest_data.pData);
				for (UINT y = 0; y < num_row; ++y)
				{
					memcpy(dest_slice + dest_data.RowPitch * y, src_slice + src_data.RowPitch * y,
						static_cast<size_t>(row_sizes_in_byte));
				}
			}
			d3d_texture_upload_heaps_->Unmap(0, nullptr);
		}

		uint32_t const dst_subres = CalcSubresource(level, array_index, 0,
			num_mip_maps_, array_size_);

		ID3D12GraphicsCommandList* cmd_list = re.D3DRenderCmdList();

		if (access_hint_ & EAH_GPU_Read)
		{
			re.ForceCPUGPUSync();
		}

		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		if (this->UpdateResourceBarrier(dst_subres, barrier, D3D12_RESOURCE_STATE_COPY_DEST))
		{
			cmd_list->ResourceBarrier(1, &barrier);
		}

		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = d3d_texture_upload_heaps_.get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = layout;

		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.pResource = d3d_resource_.get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = dst_subres;

		D3D12_BOX src_box;
		src_box.left = 0;
		src_box.top = 0;
		src_box.front = 0;
		src_box.right = width;
		src_box.bottom = 1;
		src_box.back = 1;

		cmd_list->CopyTextureRegion(&dst, x_offset, 0, 0, &src, &src_box);

		re.ForceCPUGPUSync();
	}

	void D3D12Texture::UpdateSubresource2D(uint32_t array_index, uint32_t level,
		uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height,
		void const * data, uint32_t row_pitch)
	{
		D3D12RenderEngine& re = *checked_cast<D3D12RenderEngine*>(&Context::Instance().RenderFactoryInstance().RenderEngineInstance());
		ID3D12Device* device = re.D3DDevice();

		D3D12_RESOURCE_DESC tex_desc;
		tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		tex_desc.Alignment = 0;
		tex_desc.Width = width;
		tex_desc.Height = height;
		tex_desc.DepthOrArraySize = 1;
		tex_desc.MipLevels = 1;
		tex_desc.Format = dxgi_fmt_;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		tex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		uint64_t row_sizes_in_byte;
		uint32_t num_row;
		uint64_t upload_buffer_size = 0;
		device->GetCopyableFootprints(&tex_desc, 0, 1, 0, &layout, &num_row, &row_sizes_in_byte, &upload_buffer_size);

		{
			uint8_t* p;
			d3d_texture_upload_heaps_->Map(0, nullptr, reinterpret_cast<void**>(&p));

			D3D12_SUBRESOURCE_DATA src_data;
			src_data.pData = data;
			src_data.RowPitch = row_pitch;
			src_data.SlicePitch = row_pitch * height;

			D3D12_MEMCPY_DEST dest_data;
			dest_data.pData = p + layout.Offset;
			dest_data.RowPitch = layout.Footprint.RowPitch;
			dest_data.SlicePitch = layout.Footprint.RowPitch * num_row;

			{
				uint8_t const * src_slice
					= reinterpret_cast<uint8_t const *>(src_data.pData);
				uint8_t* dest_slice = reinterpret_cast<uint8_t*>(dest_data.pData);
				for (UINT y = 0; y < num_row; ++ y)
				{
					memcpy(dest_slice + dest_data.RowPitch * y, src_slice + src_data.RowPitch * y,
						static_cast<size_t>(row_sizes_in_byte));
				}
			}
			d3d_texture_upload_heaps_->Unmap(0, nullptr);
		}

		uint32_t const dst_subres = CalcSubresource(level, array_index, 0,
			num_mip_maps_, array_size_);

		ID3D12GraphicsCommandList* cmd_list = re.D3DRenderCmdList();

		if (access_hint_ & EAH_GPU_Read)
		{
			re.ForceCPUGPUSync();
		}

		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		if (this->UpdateResourceBarrier(dst_subres, barrier, D3D12_RESOURCE_STATE_COPY_DEST))
		{
			cmd_list->ResourceBarrier(1, &barrier);
		}

		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = d3d_texture_upload_heaps_.get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = layout;

		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.pResource = d3d_resource_.get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = dst_subres;

		D3D12_BOX src_box;
		src_box.left = 0;
		src_box.top = 0;
		src_box.front = 0;
		src_box.right = width;
		src_box.bottom = height;
		src_box.back = 1;

		cmd_list->CopyTextureRegion(&dst, x_offset, y_offset, 0, &src, &src_box);

		re.ForceCPUGPUSync();
	}

	void D3D12Texture::UpdateSubresource3D(uint32_t array_index, uint32_t level,
		uint32_t x_offset, uint32_t y_offset, uint32_t z_offset,
		uint32_t width, uint32_t height, uint32_t depth,
		void const * data, uint32_t row_pitch, uint32_t slice_pitch)
	{
		D3D12RenderEngine& re = *checked_cast<D3D12RenderEngine*>(&Context::Instance().RenderFactoryInstance().RenderEngineInstance());
		ID3D12Device* device = re.D3DDevice();

		D3D12_RESOURCE_DESC tex_desc;
		tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		tex_desc.Alignment = 0;
		tex_desc.Width = width;
		tex_desc.Height = height;
		tex_desc.DepthOrArraySize = static_cast<UINT16>(depth);
		tex_desc.MipLevels = 1;
		tex_desc.Format = dxgi_fmt_;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		tex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		uint64_t row_sizes_in_byte;
		uint32_t num_row;
		uint64_t upload_buffer_size = 0;
		device->GetCopyableFootprints(&tex_desc, 0, 1, 0, &layout, &num_row, &row_sizes_in_byte, &upload_buffer_size);

		{
			uint8_t* p;
			d3d_texture_upload_heaps_->Map(0, nullptr, reinterpret_cast<void**>(&p));

			D3D12_SUBRESOURCE_DATA src_data;
			src_data.pData = data;
			src_data.RowPitch = row_pitch;
			src_data.SlicePitch = slice_pitch;

			D3D12_MEMCPY_DEST dest_data;
			dest_data.pData = p + layout.Offset;
			dest_data.RowPitch = layout.Footprint.RowPitch;
			dest_data.SlicePitch = layout.Footprint.RowPitch * num_row;

			{
				uint8_t const * src_slice
					= reinterpret_cast<uint8_t const *>(src_data.pData);
				uint8_t* dest_slice = reinterpret_cast<uint8_t*>(dest_data.pData);
				for (UINT y = 0; y < num_row; ++y)
				{
					memcpy(dest_slice + dest_data.RowPitch * y, src_slice + src_data.RowPitch * y,
						static_cast<size_t>(row_sizes_in_byte));
				}
			}
			d3d_texture_upload_heaps_->Unmap(0, nullptr);
		}

		uint32_t const dst_subres = CalcSubresource(level, array_index, 0,
			num_mip_maps_, array_size_);

		ID3D12GraphicsCommandList* cmd_list = re.D3DRenderCmdList();

		if (access_hint_ & EAH_GPU_Read)
		{
			re.ForceCPUGPUSync();
		}

		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		if (this->UpdateResourceBarrier(dst_subres, barrier, D3D12_RESOURCE_STATE_COPY_DEST))
		{
			cmd_list->ResourceBarrier(1, &barrier);
		}

		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = d3d_texture_upload_heaps_.get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = layout;

		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.pResource = d3d_resource_.get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = dst_subres;

		D3D12_BOX src_box;
		src_box.left = 0;
		src_box.top = 0;
		src_box.front = 0;
		src_box.right = width;
		src_box.bottom = height;
		src_box.back = depth;

		cmd_list->CopyTextureRegion(&dst, x_offset, y_offset, z_offset, &src, &src_box);

		re.ForceCPUGPUSync();
	}

	void D3D12Texture::UpdateSubresourceCube(uint32_t array_index, Texture::CubeFaces face, uint32_t level,
		uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height,
		void const * data, uint32_t row_pitch)
	{
		D3D12RenderEngine& re = *checked_cast<D3D12RenderEngine*>(&Context::Instance().RenderFactoryInstance().RenderEngineInstance());
		ID3D12Device* device = re.D3DDevice();

		D3D12_RESOURCE_DESC tex_desc;
		tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		tex_desc.Alignment = 0;
		tex_desc.Width = width;
		tex_desc.Height = height;
		tex_desc.DepthOrArraySize = 1;
		tex_desc.MipLevels = 1;
		tex_desc.Format = dxgi_fmt_;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		tex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		uint64_t row_sizes_in_byte;
		uint32_t num_row;
		uint64_t upload_buffer_size = 0;
		device->GetCopyableFootprints(&tex_desc, 0, 1, 0, &layout, &num_row, &row_sizes_in_byte, &upload_buffer_size);

		{
			uint8_t* p;
			d3d_texture_upload_heaps_->Map(0, nullptr, reinterpret_cast<void**>(&p));

			D3D12_SUBRESOURCE_DATA src_data;
			src_data.pData = data;
			src_data.RowPitch = row_pitch;
			src_data.SlicePitch = row_pitch * height;

			D3D12_MEMCPY_DEST dest_data;
			dest_data.pData = p + layout.Offset;
			dest_data.RowPitch = layout.Footprint.RowPitch;
			dest_data.SlicePitch = layout.Footprint.RowPitch * num_row;

			{
				uint8_t const * src_slice
					= reinterpret_cast<uint8_t const *>(src_data.pData);
				uint8_t* dest_slice = reinterpret_cast<uint8_t*>(dest_data.pData);
				for (UINT y = 0; y < num_row; ++y)
				{
					memcpy(dest_slice + dest_data.RowPitch * y, src_slice + src_data.RowPitch * y,
						static_cast<size_t>(row_sizes_in_byte));
				}
			}
			d3d_texture_upload_heaps_->Unmap(0, nullptr);
		}

		uint32_t const dst_subres = CalcSubresource(level, array_index * 6 + face - CF_Positive_X, 0,
			num_mip_maps_, array_size_);

		ID3D12GraphicsCommandList* cmd_list = re.D3DRenderCmdList();

		if (access_hint_ & EAH_GPU_Read)
		{
			re.ForceCPUGPUSync();
		}

		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		if (this->UpdateResourceBarrier(dst_subres, barrier, D3D12_RESOURCE_STATE_COPY_DEST))
		{
			cmd_list->ResourceBarrier(1, &barrier);
		}

		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = d3d_texture_upload_heaps_.get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = layout;

		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.pResource = d3d_resource_.get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = dst_subres;

		D3D12_BOX src_box;
		src_box.left = 0;
		src_box.top = 0;
		src_box.front = 0;
		src_box.right = width;
		src_box.bottom = height;
		src_box.back = 1;

		cmd_list->CopyTextureRegion(&dst, x_offset, y_offset, 0, &src, &src_box);

		re.ForceCPUGPUSync();
	}
}
