#include "pch_RulrCore.h"

#include "CaptureSet.h"
#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Utils {
#pragma mark Capture
		//----------
		AbstractCaptureSet::BaseCapture::BaseCapture() {
			this->selected.addListener(this, &BaseCapture::callbackSelectedChanged);

			this->onSerialize += [this](Json::Value & json) {
				json << this->selected;
				json << this->color;
				json["timestamp"] << chrono::system_clock::to_time_t(this->timestamp.get());
			};
			this->onDeserialize += [this](const Json::Value & json) {
				json >> this->selected;
				json >> this->color;

				{
					time_t time;
					json["timestamp"] >> time;
					this->timestamp = chrono::system_clock::from_time_t(time);
				}

				this->rebuildDateStrings();
			};

			{
				ofColor color = ofColor(200, 100, 100);
				color.setHueAngle(ofRandom(360.0));
				this->color.set(color);
			}

			this->rebuildDateStrings();
		}

		//----------
		ofxCvGui::ElementPtr AbstractCaptureSet::BaseCapture::getGuiElement() {
			auto element = make_shared<Element>();
			element->setHeight(55.0f);

			element->onDraw += [this](DrawArguments & args) {
				if (this->isSelected()) {
					ofPushStyle();
					{
						ofSetColor(50);
						ofDrawRectangle(args.localBounds);
					}
					ofPopStyle();
				}

				ofxAssets::font(ofxCvGui::getDefaultTypeface(), 22).drawString(timeString, 30, 28);
				ofxAssets::font(ofxCvGui::getDefaultTypeface(), 14).drawString(secondString, 95, 20);
				ofxAssets::font(ofxCvGui::getDefaultTypeface(), 10).drawString(dateString, 30, 45);
			};

			auto thirdHeight = element->getHeight() / 3.0f;

			{
				auto selectedButton = make_shared<Widgets::Toggle>(this->selected);
				selectedButton->setBounds(ofRectangle(2, 2, 25.0f, thirdHeight * 2.0f - 4));
				selectedButton->setCaption("");
				selectedButton->onDraw += [this](DrawArguments & args) {
					ofPushStyle();
					{
						ofSetColor(this->color);
						ofSetLineWidth(2.0f);
						if (this->isSelected()) {
							ofFill();
						}
						else {
							ofNoFill();
						}
						ofDrawCircle(args.localBounds.getCenter(), 5.0f);
					}
					ofPopStyle();
				};
				element->addChild(selectedButton);
			}

			{
				auto deleteButton = make_shared<Widgets::Button>("", [this]() {
					this->onDeletePressed.notifyListeners();
				});

				deleteButton->setBounds(ofRectangle(2, thirdHeight * 2.0f + 3, 25.0f, thirdHeight - 3 - 2));
				deleteButton->onDraw += [this](DrawArguments & args) {
					ofPushStyle();
					{
						ofSetLineWidth(2.0f);
						ofPushMatrix();
						{
							ofTranslate(args.localBounds.getCenter());
							ofDrawLine(-3, -3, +3, +3);
							ofDrawLine(-3, +3, +3, -3);
						}
						ofPopMatrix();
					}
					ofPopStyle();
				};
				element->addChild(deleteButton);
			}

			auto dataDisplayElement = this->getDataDisplay();
			{
				dataDisplayElement->setPosition(ofVec2f(120.0f, 0.0f));
				dataDisplayElement->setWidth(190.0f); // this is a little hacky
				dataDisplayElement->setHeight(element->getHeight());
				element->addChild(dataDisplayElement);
			}

			return element;
		}

		//----------
		string AbstractCaptureSet::BaseCapture::getTypeName() const {
			return "Capture";
		}

		//----------
		bool AbstractCaptureSet::BaseCapture::isSelected() const {
			return this->selected.get();
		}

		//----------
		void AbstractCaptureSet::BaseCapture::setSelected(bool selected) {
			if (selected != this->selected) {
				this->selected = selected;
				this->onSelectionChanged(selected);
			}
		}

		//----------
		ofxCvGui::ElementPtr AbstractCaptureSet::BaseCapture::getDataDisplay() {
			auto element = make_shared<ofxCvGui::Element>();
			element->onDraw += [this](ofxCvGui::DrawArguments & args) {
				ofxCvGui::Utils::drawText(this->getDisplayString(), args.localBounds.x, args.localBounds.y, false);
			};
			return element;
		}

		//----------
		void AbstractCaptureSet::BaseCapture::callbackSelectedChanged(bool & value) {
			this->onSelectionChanged.notifyListeners(value);
		}

		//----------
		void AbstractCaptureSet::BaseCapture::rebuildDateStrings() {
			time_t time = chrono::system_clock::to_time_t(this->timestamp.get());

			{
				char buff[10];
				strftime(buff, 20, "%H:%M", localtime(&time));
				this->timeString = string(buff);
			}

			{
				char buff[5];
				strftime(buff, 20, ":%S", localtime(&time));
				this->secondString = string(buff);
			}

			{
				char buff[20];
				strftime(buff, 20, "%a %Y.%m.%d", localtime(&time));
				this->dateString = string(buff);
			}

		}
#pragma mark AbstractCaptureSet
		//----------
		AbstractCaptureSet::AbstractCaptureSet() {
			RULR_SERIALIZE_LISTENERS;

			this->listView = Panels::makeWidgets();
			this->listView->setHeight(400.0f);
			this->listView->onUpdate += [this](ofxCvGui::UpdateArguments &) {
				if (this->viewDirty) {
					this->listView->clear();
					for (auto capture : this->captures) {
						this->listView->add(capture->getGuiElement());
					}
					this->viewDirty = false;
				}
			};
			this->listView->onDraw += [this](DrawArguments & args) {
				ofPushStyle();
				{
					ofNoFill();
					ofSetLineWidth(1.0f);
					ofDrawRectangle(args.localBounds);
				}
				ofPopStyle();
			};
			this->listView->setScissorEnabled(true);
		}

		//----------
		AbstractCaptureSet::~AbstractCaptureSet() {
			this->clear();
		}

		//----------
		void AbstractCaptureSet::add(shared_ptr<BaseCapture> capture) {
			auto captureWeak = weak_ptr<BaseCapture>(capture);
			capture->onDeletePressed += [captureWeak, this]() {
				auto capture = captureWeak.lock();
				if (capture) {
					this->remove(capture);
				}
			};

			capture->onSelectionChanged += [captureWeak, this](bool selection) {
				if (!this->getIsMultipleSelectionAllowed() && selection) {
					auto selectionSet = this->getSelectionUntyped();
					auto selectedCapture = captureWeak.lock();
					for (auto otherCapture : selectionSet) {
						if (otherCapture != selectedCapture && otherCapture->isSelected()) {
							otherCapture->setSelected(false);
						}
					}
				}
				this->onSelectionChanged.notifyListeners();
			};

			this->onSelectionChanged.notifyListeners();

			this->captures.push_back(capture);
			this->viewDirty = true;
		}

		//----------
		void AbstractCaptureSet::remove(shared_ptr<BaseCapture> capture) {
			auto findCapture = find(this->captures.begin(), this->captures.end(), capture);
			if (findCapture != this->captures.end()) {
				if (capture) {
					capture->setSelected(false); // to notify upwards
					capture->onDeletePressed.removeListeners(this);
					capture->onSelectionChanged.removeListeners(this);
				}
				this->captures.erase(findCapture);
				this->viewDirty = true;
			}
		}

		//----------
		void AbstractCaptureSet::clear() {
			while (!this->captures.empty()) {
				this->remove(* this->captures.begin());
			}
		}

		//----------
		void AbstractCaptureSet::select(shared_ptr<BaseCapture> capture) {
			capture->setSelected(true);
		}

		//----------
		void AbstractCaptureSet::selectAll() {
			for (auto capture : this->captures) {
				capture->setSelected(true);
			}
		}

		//----------
		void AbstractCaptureSet::selectNone() {
			for (auto capture : this->captures) {
				capture->setSelected(false);
			}
		}

		//----------
		void AbstractCaptureSet::populateWidgets(shared_ptr<ofxCvGui::Panels::Widgets> widgetsPanel) {
			widgetsPanel->add(this->listView);

			widgetsPanel->addLiveValue<int>("Count", [this]() {
				return this->captures.size();
			});

			widgetsPanel->addButton("Clear", [this]() {
				this->clear();
			});

			{
				auto selectionSelector = widgetsPanel->addMultipleChoice("Selection", { "All", "None" });
				selectionSelector->setAllowNullSelection(true);
				selectionSelector->onValueChange += [this](int selectionIndex) {
					switch (selectionIndex) {
					case 0:
						this->selectAll();
						break;
					case 1:
						this->selectNone();
						break;
					default:
						break;
					}
				};
				auto selectionSelectorWeak = weak_ptr<Widgets::MultipleChoice>(selectionSelector);
				selectionSelector->onUpdate += [this, selectionSelectorWeak](UpdateArguments &) {
					auto selectionSelector = selectionSelectorWeak.lock();
					if (selectionSelector) {
						if (this->captures.empty()) {
							selectionSelector->setSelection(-1);
						}
						bool allSelected = true;
						bool noneSelected = true;
						for (auto capture : this->captures) {
							if (capture->isSelected()) {
								noneSelected = false;
							}
							else {
								allSelected = false;
							}
						}
						if (allSelected) {
							selectionSelector->setSelection(0);
						}
						else if (noneSelected) {
							selectionSelector->setSelection(1);
						}
						else {
							selectionSelector->setSelection(-1);
						}
					}
				};
			}

			widgetsPanel->addSpacer();
		}

		//----------
		void AbstractCaptureSet::serialize(Json::Value & json) {
			auto & jsonCaptures = json["captures"];
			int index = 0;
			for (auto capture : this->captures) {
				capture->serialize(jsonCaptures[index++]);
			}
		}

		//----------
		void AbstractCaptureSet::deserialize(const Json::Value & json) {
			this->captures.clear();
			auto & jsonCaptures = json["captures"];
			for (const auto & jsonCapture : jsonCaptures) {
				auto capture = this->makeEmpty();
				capture->deserialize(jsonCapture);
				this->add(capture); //ensure event listeners are attached
			}
		}

		//----------
		vector<shared_ptr<ofxRulr::Utils::AbstractCaptureSet::BaseCapture>> AbstractCaptureSet::getSelectionUntyped() const {
			auto selection = this->captures;
			for (auto it = selection.begin(); it != selection.end(); ) {
				if (!(*it)->isSelected()) {
					it = selection.erase(it);
				}
				else {
					it++;
				}
			}
			return selection;
		}

		//----------
		vector<shared_ptr<ofxRulr::Utils::AbstractCaptureSet::BaseCapture>> AbstractCaptureSet::getAllCapturesUntyped() const {
			return this->captures;
		}
	}
}