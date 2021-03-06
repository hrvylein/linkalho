#include <switch.h>
#include "confirm_page.hpp"
#include <borealis.hpp>
#include "utils.hpp"
#include <algorithm>
#include "reboot_payload.h"

ConfirmPage::ConfirmPage(brls::StagedAppletFrame* frame, const std::string& text, bool reboot, bool canUseLed): reboot(reboot), canUseLed(canUseLed)
{
    auto payloadFile = getPayload();

    this->button = (new brls::Button(reboot ? brls::ButtonStyle::BORDERLESS: brls::ButtonStyle::PLAIN))->setLabel(reboot ? (payloadFile.empty() ? "Reboot" : "Reboot to payload"): "Continue");

    this->button->setParent(this);
    this->button->getClickEvent()->subscribe([frame, this, payloadFile](View* view) {
        if (!frame->isLastStage())
            frame->nextStage();
        else if (this->reboot) {
            if (payloadFile.empty() || !rebootToPayload(payloadFile.c_str())) {
                attemptForceReboot();
            }
            brls::Application::popView();
        }
    });

    this->label = new brls::Label(brls::LabelStyle::DIALOG, text, true);
    this->label->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->label->setParent(this);

    if (!reboot) {
        this->registerAction("Back", brls::Key::B, [this] {
            brls::Application::popView();
            return true;
        });
    }
}

void ConfirmPage::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx)
{
    if (!this->reboot) {
        auto end = std::chrono::high_resolution_clock::now();
        auto missing = std::max(3l - std::chrono::duration_cast<std::chrono::seconds>(end - start).count(), 0l);
        auto text =  std::string(this->reboot ? "Reboot": "Continue");
        if (missing > 0) {
            this->button->setLabel(text + " (" + std::to_string(missing) + ")");
            this->button->setState(brls::ButtonState::DISABLED);
        } else {
            this->button->setLabel(text);
            this->button->setState(brls::ButtonState::ENABLED);
        }
        this->button->invalidate();
    }
    this->label->frame(ctx);
    this->button->frame(ctx);
}

brls::View* ConfirmPage::getDefaultFocus()
{
    return this->button;
}

void ConfirmPage::layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash)
{
    this->label->setWidth(this->width);
    this->label->invalidate(true);
    // this->label->setBackground(brls::ViewBackground::DEBUG);
    this->label->setBoundaries(
        this->x + this->width / 2 - this->label->getWidth() / 2,
        this->y + (this->height - this->label->getHeight() - this->y - style->CrashFrame.buttonHeight)/2,
        this->label->getWidth(),
        this->label->getHeight());

    this->button->setBoundaries(
        this->x + this->width / 2 - style->CrashFrame.buttonWidth / 2,
        this->y + (this->height-style->CrashFrame.buttonHeight*3),
        style->CrashFrame.buttonWidth,
        style->CrashFrame.buttonHeight);
    this->button->invalidate();

    start = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(150);
    if (this->canUseLed) {
        if (!this->reboot) {
            HidsysNotificationLedPattern pattern = getBreathePattern();
            sendLedPattern(pattern);
        } else {
            HidsysNotificationLedPattern pattern = getConfirmPattern();
            sendLedPattern(pattern);
        }
    }
}

void ConfirmPage::willDisappear(bool resetState)
{
    if (!this->reboot) {
        HidsysNotificationLedPattern pattern = getClearPattern();
        sendLedPattern(pattern);
    }
}

ConfirmPage::~ConfirmPage()
{
    delete this->label;
    delete this->button;
}
