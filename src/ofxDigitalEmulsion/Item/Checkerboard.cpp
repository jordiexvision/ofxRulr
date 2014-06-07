#include "Checkerboard.h"
#include "../../../addons/ofxCvGui2/src/ofxCvGui.h"
#include "ofxCvMin.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		Checkerboard::Checkerboard() {
			this->sizeX.set("Size X", 9.0f, 2.0f, 20.0f);
			this->sizeY.set("Size Y", 5.0f, 2.0f, 20.0f);
			this->spacing.set("Spacing [m]", 0.025f, 0.001f, 1.0f);
			this->updatePreviewMesh();
		}

		//----------
		string Checkerboard::getTypeName() const {
			return "Checkerboard";
		}

		//----------
		shared_ptr<ofxCvGui::Widgets::Slider> addSlider(ofParameter<float> & parameter, ElementGroupPtr inspector) {
			auto slider = Widgets::Slider::make(parameter);
			slider->addIntValidator();
			inspector->add(slider);
			return slider;
		}

		//----------
		ofxCvGui::PanelPtr Checkerboard::getView() {
			auto view = MAKE(ofxCvGui::Panels::World);
			view->onDrawWorld += [this] (ofCamera &) {
				this->previewMesh.draw();
			};
			view->setGridEnabled(false);
			view->getCamera().setCursorDraw(true, this->spacing / 5.0f);

			auto & camera = view->getCamera();
			auto distance = this->spacing * MAX(this->sizeX, this->sizeY);
			camera.setPosition(-distance, distance, -distance);
			camera.lookAt(ofVec3f());

			return view;
		}

		//----------
		void Checkerboard::serialize(Json::Value & json) {
			Utils::Serializable::serialize(this->sizeX, json);
			Utils::Serializable::serialize(this->sizeY, json);
			Utils::Serializable::serialize(this->spacing, json);
		}

		//----------
		void Checkerboard::deserialize(Json::Value & json) {
			Utils::Serializable::deserialize(this->sizeX, json);
			Utils::Serializable::deserialize(this->sizeY, json);
			Utils::Serializable::deserialize(this->spacing, json);

			this->updatePreviewMesh();
		}

		//----------
		cv::Size Checkerboard::getSize() const {
			return cv::Size(this->sizeX, this->sizeY);
		}

		//----------
		vector<cv::Point3f> Checkerboard::getObjectPoints() const {
			return ofxCv::makeCheckerboardPoints(this->getSize(), this->spacing);
		}

		//----------
		void Checkerboard::populateInspector2(ElementGroupPtr inspector) {
			auto sliderCallback = [this] (ofParameter<float> &) {
				this->updatePreviewMesh();
			};
			
			addSlider(this->sizeX, inspector)->onValueChange += sliderCallback;
			addSlider(this->sizeY, inspector)->onValueChange += sliderCallback;
			inspector->add(Widgets::Title::make("NB : The chessboard size is defined by the amount of inner corners", Widgets::Title::Level::H3));
			inspector->add(Widgets::Spacer::make());
			
			auto spacingSlider = Widgets::Slider::make(this->spacing);
			spacingSlider->onValueChange += sliderCallback;
			inspector->add(spacingSlider);
		}

		//----------
		void Checkerboard::updatePreviewMesh() {
			this->previewMesh = ofxCv::makeCheckerboardMesh(this->getSize(), this->spacing);
		}
	}
}