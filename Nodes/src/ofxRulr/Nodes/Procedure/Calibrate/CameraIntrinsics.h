#pragma once

#include "../Base.h"
#include "ofxCvMin.h"
#include "ofxRulr/Nodes/Item/Board.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class CameraIntrinsics : public Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();

						string getDisplayString() const override;
						void drawWorld();
						void drawOnImage() const;

						vector<ofVec2f> pointsImageSpace;
						vector<ofVec3f> pointsObjectSpace;

						ofParameter<float> imageWidth{ "Image width", 0.0f };
						ofParameter<float> imageHeight{ "Image height", 0.0f };

						ofParameter<ofMatrix4x4> extrsinsics{ "Extrinsics", ofMatrix4x4() };
						ofParameter<float> reprojectionError{ "Reprojection error", 0.0f };
					protected:
						void serialize(Json::Value &);
						void deserialize(const Json::Value &);
					};

					CameraIntrinsics();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;

					void update();
					void drawWorld();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					bool getRunFinderEnabled() const;
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void addCapture(bool triggeredFromTetheredCapture);
					void findBoard();
					void calibrate();

					shared_ptr<ofxCvGui::Panels::BaseImage> view;
					ofTexture preview;

					Utils::CaptureSet<Capture> captures;

					vector<ofVec2f> currentImagePoints;
					vector<ofVec3f> currentObjectPoints;

					ofParameter<float> error{ "Reprojection error [px]", 0.0f };

					struct : ofParameterGroup {
						ofParameter<WhenDrawWorld> drawBoards{ "Draw boards", WhenDrawWorld::Selected };
						struct : ofParameterGroup {
							ofParameter<bool> checkAllIncomingFrames{ "Check all incoming frames", true };
							ofParameter<bool> tetheredShootEnabled{ "Tethered shoot enabled", true };
							ofParameter<FindBoardMode> findBoardMode{ "Mode", FindBoardMode::Optimized };

							PARAM_DECLARE("Capture", checkAllIncomingFrames, tetheredShootEnabled, findBoardMode);
						} capture;
						PARAM_DECLARE("CameraIntrinsics", drawBoards, capture);
					} parameters;
					
					bool isFrameNew = false;
				};
			}
		}
	}
}