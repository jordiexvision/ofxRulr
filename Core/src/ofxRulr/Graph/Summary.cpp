#include "pch_RulrCore.h"
#include "Summary.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Graph {
		//----------
		Summary::Summary() :
		world() {
			RULR_NODE_INIT_LISTENER;
			this->world = nullptr;
		}

		//----------
		string Summary::getTypeName() const {
			return "Summary";
		}

		//----------
		void Summary::init() {
			RULR_NODE_UPDATE_LISTENER;
			RULR_NODE_INSPECTOR_LISTENER;
			RULR_NODE_SERIALIZATION_LISTENERS;

			this->view = MAKE(ofxCvGui::Panels::World);
			this->view->onDraw.addListener([this](ofxCvGui::DrawArguments &) {
				ofBackgroundGradient(40, 0);
			}, this, -1);
			this->view->onDrawWorld += [this](ofCamera &) {
				if (this->parameters.grid.enabled) {
					this->drawGrid();
				}
				if (this->world) {
					auto & world = *this->world;
					for (const auto node : world) {
						node->drawWorld();
					}
				}
			};
			auto & camera = this->view->getCamera();
			camera.setNearClip(0.01f);
			camera.setFarClip(10000.0f);
			this->camera = &camera;

			auto wasArbTex = ofGetUsingArbTex();
			ofDisableArbTex();
			this->grid = new ofTexture();
			this->grid->enableMipmap();
			this->grid->loadData(ofxAssets::image("ofxRulr::grid-10").getPixels());
			this->grid->setTextureWrap(GL_REPEAT, GL_REPEAT);
			this->grid->setTextureMinMagFilter(GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST);
			if (wasArbTex) ofEnableArbTex();

#ifdef OFXCVGUI_USE_OFXGRABCAM
			this->parameters.showCursor.addListener(this, &Summary::callbackShowCursor);
#endif
			this->parameters.grid.dark.addListener(this, &Summary::callbackGridDark);

			this->view->setGridEnabled(false);
		}

		//----------
		void Summary::setWorld(const Utils::Set<Nodes::Base> & world) {
			this->world = &world;
		}

		//----------
		void Summary::update() {
			auto & camera = view->getCamera();
			this->light.setPosition(camera.getPosition());
			this->light.lookAt(camera.getCursorWorld());
		}

		//----------
		ofxCvGui::PanelPtr Summary::getPanel() {
			return this->view;
		}

		//----------
		void Summary::serialize(Json::Value & json) {
			Utils::Serializable::serialize(json, this->parameters);

			auto & camera = this->view->getCamera();
			auto & cameraJson = json["Camera"];
			cameraJson["position"] << camera.getPosition();
			cameraJson["orientation"] << camera.getOrientationQuat().asVec4(); //cast as ofVec4f since ofQuaternion doesn't have serialisation
		}

		//----------
		void Summary::deserialize(const Json::Value & json) {
			Utils::Serializable::deserialize(json, this->parameters);

			auto & camera = this->view->getCamera();
			if (json.isMember("Camera")) {
				auto & cameraJson = json["Camera"];
				ofVec3f position;
				cameraJson["position"] >> position;
				camera.setPosition(position);

				ofQuaternion orientation;
				cameraJson["orientation"] >> (ofVec4f&) orientation;
				camera.setOrientation(orientation);
			}
			else {
				camera.setPosition(this->parameters.grid.roomMinimum.get() * ofVec3f(0.0f, 1.0f, 1.0f));
				camera.lookAt(this->parameters.grid.roomMaximum.get() * ofVec3f(0.0f, 1.0f, 1.0f), ofVec3f(0, -1, 0));
				camera.move(ofVec3f()); // nudge camera to update
			}
		}

		//----------
		void Summary::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
			auto inspector = inspectArguments.inspector;
			
#ifdef OFXCVGUI_USE_OFXGRABCAM
			inspector->add(new Widgets::Title("Cursor", Widgets::Title::Level::H3));
			{
				inspector->add(new Widgets::Toggle(this->parameters.showCursor));

				auto formatMeters = [](float distance) {
					stringstream formatted;

					formatted << floor(distance) << "m ";
					distance = fmod(distance * 100.0f, 100.0f);
					formatted << floor(distance) << "cm ";
					distance = fmod(distance * 10.0f, 10.0f);
					formatted << floor(distance) << "mm";
					return formatted.str();
				};
				inspector->add(new Widgets::LiveValue<string>("Position X", [this, formatMeters]() {
					return formatMeters(this->view->getCamera().getCursorWorld().x);
				}));
				inspector->add(new Widgets::LiveValue<string>("Position Y", [this, formatMeters]() {
					return formatMeters(this->view->getCamera().getCursorWorld().y);
				}));
				inspector->add(new Widgets::LiveValue<string>("Position Z", [this, formatMeters]() {
					return formatMeters(this->view->getCamera().getCursorWorld().z);
				}));
			}
#endif
			if (this->camera) {
				inspector->addEditableValue<ofVec3f>("Camera position", [this]() {
					return this->camera->getPosition();
				}, [this](string & positionString) {
					if (!positionString.empty()) {
						stringstream stream(positionString);
						ofVec3f newPosition;
						stream >> newPosition;
						this->camera->setPosition(newPosition);
					}
				});
			}

			inspector->addParameterGroup(this->parameters.grid);
		}


#ifdef OFXCVGUI_USE_OFXGRABCAM
		//----------
		void Summary::callbackShowCursor(bool & showCursor) {
			this->view->setCursorEnabled(showCursor);
		}
#endif
		//----------
		void Summary::callbackGridDark(bool &) {
			if (this->parameters.grid.dark) {
				this->grid->loadData(ofxAssets::image("ofxRulr::grid-10-dark").getPixels());
			}
			else {
				this->grid->loadData(ofxAssets::image("ofxRulr::grid-10").getPixels());
			}
		}

		//----------
		void Summary::drawGrid() {
			const auto & roomMinimum = this->parameters.grid.roomMinimum.get();
			const auto & roomMaximum = this->parameters.grid.roomMaximum.get();
			const auto roomSpan = roomMaximum - roomMinimum;
			auto & camera = this->view->getCamera();

			//--
			//axes
			//--
			//
			ofPushStyle();
			ofSetLineWidth(2.0f);
			//
			//x
			ofSetColor(200, 100, 100);
			ofPushMatrix();
			ofTranslate(0, roomMaximum.y, roomMaximum.z);
			ofDrawLine(roomMinimum.x, 0, roomMaximum.x, 0);
			ofPopMatrix();
			//
			//y
			ofSetColor(100, 200, 100);
			ofDrawLine(0, roomMaximum.y, roomMaximum.z, 0, roomMinimum.y, roomMaximum.z);
			//
			//z
			ofSetColor(100, 200, 100);
			ofDrawLine(0, roomMaximum.y, roomMaximum.z, 0, roomMaximum.y, roomMinimum.z);
			//
			ofPopStyle();
			//
			//--



#ifdef OFXCVGUI_USE_OFXGRABCAM
			//--
			//cursor lines
			//--
			//
			ofPushStyle();
			{
				const auto cursorPosition = camera.getCursorWorld();
				//
				if (this->parameters.grid.dark) {
					ofSetColor(100);
				}
				else {
					ofSetColor(0);
				}
				ofSetLineWidth(2.0f);
				//front wall
				ofDrawLine(cursorPosition.x, roomMaximum.y, roomMinimum.z, cursorPosition.x, roomMinimum.y, roomMinimum.z); //x
				ofDrawLine(roomMinimum.x, cursorPosition.y, roomMinimum.z, roomMaximum.x, cursorPosition.y, roomMinimum.z); //y
				//back wall
				ofDrawLine(cursorPosition.x, roomMaximum.y, roomMaximum.z, cursorPosition.x, roomMinimum.y, roomMaximum.z); //x
				ofDrawLine(roomMinimum.x, cursorPosition.y, roomMaximum.z, roomMaximum.x, cursorPosition.y, roomMaximum.z); //y
				//
				//floor
				ofDrawLine(cursorPosition.x, roomMaximum.y, roomMinimum.z, cursorPosition.x, roomMaximum.y, roomMaximum.z); //x
				ofDrawLine(roomMinimum.x, roomMaximum.y, cursorPosition.z, roomMaximum.x, roomMaximum.y, cursorPosition.z); //z
				//ceiling
				ofDrawLine(cursorPosition.x, roomMinimum.y, roomMinimum.z, cursorPosition.x, roomMinimum.y, roomMaximum.z); //x
				ofDrawLine(roomMinimum.x, roomMinimum.y, cursorPosition.z, roomMaximum.x, roomMinimum.y, cursorPosition.z); //z
				//
				//left wall
				ofDrawLine(roomMinimum.x, cursorPosition.y, roomMinimum.z, roomMinimum.x, cursorPosition.y, roomMaximum.z); //y
				ofDrawLine(roomMinimum.x, roomMinimum.y, cursorPosition.z, roomMinimum.x, roomMaximum.y, cursorPosition.z); //z
				//right wall
				ofDrawLine(roomMaximum.x, cursorPosition.y, roomMinimum.z, roomMaximum.x, cursorPosition.y, roomMaximum.z); //y
				ofDrawLine(roomMaximum.x, roomMinimum.y, cursorPosition.z, roomMaximum.x, roomMaximum.y, cursorPosition.z); //z
				//
			}
			ofPopStyle();
			//
			//--
#endif



			//--
			//grid
			//--
			//
			glEnable(GL_CULL_FACE);
			this->grid->bind();
			{
				//
				//front/back walls
				ofPushMatrix();
				{
					this->light.enable();

					auto planeXY = ofPlanePrimitive(roomSpan.x, roomSpan.y, 2, 2);
					planeXY.mapTexCoords(roomMinimum.x, roomSpan.y, roomMaximum.x, 0);

					//front
					glCullFace(GL_FRONT);
					ofTranslate(roomMinimum.x + roomSpan.x * 0.5, roomMaximum.y - roomSpan.y * 0.5, roomMinimum.z);
					planeXY.draw();

					//back
					glCullFace(GL_BACK);
					ofTranslate(0, 0, roomSpan.z);
					planeXY.draw();

					this->light.disable();
					ofDisableLighting();
				}
				ofPopMatrix();
				//
				//floor/ceiling
				ofPushMatrix();
				{
					auto planeXZ = ofPlanePrimitive(roomSpan.x, roomSpan.z, 2, 2);
					planeXZ.mapTexCoords(roomMinimum.x, roomSpan.z, roomMaximum.x, 0);

					ofTranslate(roomMinimum.x + roomSpan.x * 0.5, roomMaximum.y, 0);
					ofRotate(90, -1, 0, 0);

					//floor
					glCullFace(GL_BACK);
					ofTranslate(0, roomSpan.z * 0.5 - roomMaximum.z, 0);
					planeXZ.draw();

					//ceiling
					glCullFace(GL_FRONT);
					ofTranslate(0, 0, -roomSpan.y);
					planeXZ.draw();
				}
				ofPopMatrix();
				//
				//side walls
				ofPushMatrix();
				{
					auto planeYZ = ofPlanePrimitive(roomSpan.z, roomSpan.y, 2, 2);
					planeYZ.mapTexCoords(roomMinimum.z, roomSpan.y, roomMaximum.z, 0);

					ofTranslate(0, roomMaximum.y - roomSpan.y * 0.5, roomMaximum.z - roomSpan.z * 0.5);
					ofRotate(-90, 0, 1, 0);

					//left wall
					glCullFace(GL_BACK);
					ofTranslate(0, 0, -roomMinimum.x);
					planeYZ.draw();

					//right wall
					glCullFace(GL_FRONT);
					ofTranslate(0, 0, -roomSpan.x);
					planeYZ.draw();
				}
				ofPopMatrix();
				//
			}
			this->grid->unbind();
			glDisable(GL_CULL_FACE);
			//
			//--

		}
	}
}