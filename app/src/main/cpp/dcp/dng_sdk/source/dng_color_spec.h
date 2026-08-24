

#ifndef __dng_color_spec__
#define __dng_color_spec__

#include "dng_classes.h"
#include "dng_matrix.h"
#include "dng_types.h"
#include "dng_xy_coord.h"

dng_matrix_3by3 MapWhiteMatrix (const dng_xy_coord &white1,
								const dng_xy_coord &white2);
						   

class dng_color_spec
	{
	
	private:
	
		uint32 fChannels;

		real64 fTemperature1;
		real64 fTemperature2;

		dng_illuminant_data fLight1;
		dng_illuminant_data fLight2;
		dng_illuminant_data fLight3;

		dng_matrix fColorMatrix1;
		dng_matrix fColorMatrix2;
		dng_matrix fColorMatrix3;
		
		dng_matrix fForwardMatrix1;
		dng_matrix fForwardMatrix2;
		dng_matrix fForwardMatrix3;
		
		dng_matrix fReductionMatrix1;
		dng_matrix fReductionMatrix2;
		dng_matrix fReductionMatrix3;
		
		dng_matrix fCameraCalibration1;
		dng_matrix fCameraCalibration2;
		dng_matrix fCameraCalibration3;
		
		dng_matrix fAnalogBalance;
		
		dng_xy_coord fWhiteXY;
		
		dng_vector fCameraWhite;
		dng_matrix fCameraToPCS;
		
		dng_matrix fPCStoCamera;

		

		uint32 fNumIlluminants = 1;
		
	public:

		
		

		dng_color_spec (const dng_negative &negative,
						const dng_camera_profile *profile,
						bool allowStubbed = false);
						
		virtual ~dng_color_spec ()
			{
			}

		
		

		uint32 Channels () const
			{
			return fChannels;
			}

		
		

		void SetWhiteXY (const dng_xy_coord &white);
		
		
		

		const dng_xy_coord & WhiteXY () const;

		
		
		

		const dng_vector & CameraWhite () const;
			
		
		
		

		const dng_matrix & CameraToPCS () const;

		
		
		

		const dng_matrix & PCStoCamera () const;

		
		
		
		
		
		
		

		dng_xy_coord NeutralToXY (const dng_vector &neutral);

	private:
	
		dng_matrix FindXYZtoCamera (const dng_xy_coord &white,
									dng_matrix *forwardMatrix = NULL,
									dng_matrix *reductionMatrix = NULL,
									dng_matrix *cameraCalibration = NULL);

		dng_matrix FindXYZtoCamera_SingleOrDual (const dng_xy_coord &white,
												 dng_matrix *forwardMatrix = NULL,
												 dng_matrix *reductionMatrix = NULL,
												 dng_matrix *cameraCalibration = NULL);
		
		dng_matrix FindXYZtoCamera_Triple (const dng_xy_coord &white,
										   dng_matrix *forwardMatrix = NULL,
										   dng_matrix *reductionMatrix = NULL,
										   dng_matrix *cameraCalibration = NULL);

		void CalculateTripleIlluminantWeights (const dng_xy_coord &white,
											   real64 &w1,
											   real64 &w2,
											   real64 &w3) const;
		
	};

#endif

