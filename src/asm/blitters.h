
#ifndef _OPTIMISED_BLITTERS_H_
#define _OPTIMISED_BLITTERS_H_

#ifdef __cplusplus
	extern "C" {
#endif 

void blit_windowed_dirty_mmx (uint8 *src, 
							  uint8 *dirty, 
							  uint8 *dst, 
							  size_t size, 
							  int32 bpp);
							 

void blit_2x_windowed_dirty_mmx(uint8 *src, 
								uint8 *dirty, 
								uint8 *dst, 
								size_t size, 				
								int32 bpp, 
								int32 rowbytes);
								
void blit_2x_dirty_mmx (uint8 *dest, 
						uint8 *source, 
						uint8 *dirty, 
						int32 rowbytes, 
						size_t size);
								


void blit_overlay (uint8 *dest, 
				   uint8 *source, 
				   size_t size, 
				   uint32 *y, 
				   uint32 *ycbcr);	
				   
#ifdef __cplusplus
}
#endif


#endif // _OPTIMISED_BLITTERS_H_
