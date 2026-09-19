#include "TSupFDetMonGui.h"

#include "SupFDetMonNames.h"
#include "TSupFDetMonClient.h"

#include <TCanvas.h>
#include <TGButton.h>
#include <TGClient.h>
#include <TGComboBox.h>
#include <TGFrame.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGTextEntry.h>
#include <TRootEmbeddedCanvas.h>
#include <TH1D.h>

#include <iostream>
#include <string>

ClassImp(TSupFDetMonGui)

TSupFDetMonGui::TSupFDetMonGui(const TGWindow* parent, UInt_t width, UInt_t height,
                                   std::string host, int port)
    : TGMainFrame(parent, width, height)
{
    SetCleanup(kDeepCleanup);
    SetWindowName("SupFDetMon Client");

    auto* connection = new TGGroupFrame(this, "Connection", kHorizontalFrame);
    fHostEntry = new TGTextEntry(connection, host.c_str());
    fHostEntry->Resize(180, 28);
    fPortEntry = new TGNumberEntry(connection, port, 6, -1, TGNumberFormat::kNESInteger,
                                   TGNumberFormat::kNEANonNegative,
                                   TGNumberFormat::kNELLimitMinMax, 1, 65535);
    fConnectButton = new TGTextButton(connection, "&Connect");
    fDisconnectButton = new TGTextButton(connection, "&Disconnect");
    fStatusLabel = new TGLabel(connection, "Disconnected");

    connection->AddFrame(new TGLabel(connection, "Host:"),
                         new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 5, 4, 5, 5));
    connection->AddFrame(fHostEntry, new TGLayoutHints(kLHintsCenterY, 0, 15, 5, 5));
    connection->AddFrame(new TGLabel(connection, "Port:"),
                         new TGLayoutHints(kLHintsCenterY, 0, 4, 5, 5));
    connection->AddFrame(fPortEntry, new TGLayoutHints(kLHintsCenterY, 0, 15, 5, 5));
    connection->AddFrame(fConnectButton, new TGLayoutHints(kLHintsCenterY, 0, 8, 5, 5));
    connection->AddFrame(fDisconnectButton, new TGLayoutHints(kLHintsCenterY, 0, 20, 5, 5));
    connection->AddFrame(fStatusLabel, new TGLayoutHints(kLHintsCenterY, 0, 5, 5, 5));
    AddFrame(connection, new TGLayoutHints(kLHintsExpandX, 8, 8, 8, 4));

    auto* body = new TGHorizontalFrame(this);

    auto* controls = new TGGroupFrame(body, "Select Histogram", kVerticalFrame);
    controls->Resize(300, 500);

    auto* fcRow = new TGHorizontalFrame(controls);
    fcRow->AddFrame(new TGLabel(fcRow, "Field Cage:"), new TGLayoutHints(kLHintsCenterY, 2, 8, 4, 4));
    fFieldCageCombo = new TGComboBox(fcRow);
    for (int fc = 1; fc <= SupFDetMon::kNFieldCages; ++fc)
        fFieldCageCombo->AddEntry(("FC" + std::to_string(fc)).c_str(), fc);
    fFieldCageCombo->Select(1);
    fFieldCageCombo->Resize(150, 24);
    fcRow->AddFrame(fFieldCageCombo, new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));
    controls->AddFrame(fcRow, new TGLayoutHints(kLHintsExpandX));

    auto* adcRow = new TGHorizontalFrame(controls);
    adcRow->AddFrame(new TGLabel(adcRow, "ADC Channel:"), new TGLayoutHints(kLHintsCenterY, 2, 8, 4, 4));
    fAdcCombo = new TGComboBox(adcRow);
    for (int adc = 0; adc < SupFDetMon::kNAdcChannels; ++adc)
        fAdcCombo->AddEntry(("ADC" + std::to_string(adc)).c_str(), adc);
    fAdcCombo->Select(0);
    fAdcCombo->Resize(150, 24);
    adcRow->AddFrame(fAdcCombo, new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));
    controls->AddFrame(adcRow, new TGLayoutHints(kLHintsExpandX));

    controls->AddFrame(new TGLabel(controls, "Histogram name:"),
                       new TGLayoutHints(kLHintsLeft, 2, 2, 12, 2));
    fHistogramEntry = new TGTextEntry(controls);
    fHistogramEntry->SetEnabled(kFALSE);
    controls->AddFrame(fHistogramEntry, new TGLayoutHints(kLHintsExpandX, 2, 2, 2, 10));

    fDrawButton = new TGTextButton(controls, "&Draw");
    fClearButton = new TGTextButton(controls, "C&lear");
    fClearAllButton = new TGTextButton(controls, "Clear &All");
    controls->AddFrame(fDrawButton, new TGLayoutHints(kLHintsExpandX, 2, 2, 8, 4));
    controls->AddFrame(fClearButton, new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));
    controls->AddFrame(fClearAllButton, new TGLayoutHints(kLHintsExpandX, 2, 2, 4, 4));

    body->AddFrame(controls, new TGLayoutHints(kLHintsLeft | kLHintsExpandY, 8, 4, 4, 8));

    auto* display = new TGGroupFrame(body, "Histogram", kVerticalFrame);
    fCanvas = new TRootEmbeddedCanvas("SupFDetMonCanvas", display, 850, 560);
    display->AddFrame(fCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 4, 4, 4, 4));
    body->AddFrame(display, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 4, 8, 4, 8));

    AddFrame(body, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

    fConnectButton->Connect("Clicked()", "TSupFDetMonGui", this, "ConnectServer()");
    fDisconnectButton->Connect("Clicked()", "TSupFDetMonGui", this, "DisconnectServer()");
    fFieldCageCombo->Connect("Selected(Int_t)", "TSupFDetMonGui", this, "SelectionChanged(Int_t)");
    fAdcCombo->Connect("Selected(Int_t)", "TSupFDetMonGui", this, "SelectionChanged(Int_t)");
    fDrawButton->Connect("Clicked()", "TSupFDetMonGui", this, "DrawSelected()");
    fClearButton->Connect("Clicked()", "TSupFDetMonGui", this, "ClearSelected()");
    fClearAllButton->Connect("Clicked()", "TSupFDetMonGui", this, "ClearAllHistograms()");

    UpdateHistogramName();
    SetConnectedUi(false);

    MapSubwindows();
    Resize(GetDefaultSize());
    MapWindow();
}

TSupFDetMonGui::~TSupFDetMonGui()
{
    if (fClient) fClient->Disconnect();
}

std::string TSupFDetMonGui::SelectedHistogramName() const
{
    const int fc = fFieldCageCombo->GetSelected();
    const int adc = fAdcCombo->GetSelected();
    return SupFDetMon::MusicAdcHistogramName(fc, adc);
}

void TSupFDetMonGui::UpdateHistogramName()
{
    fHistogramEntry->SetText(SelectedHistogramName().c_str());
}

void TSupFDetMonGui::SetConnectedUi(bool connected)
{
    fConnectButton->SetEnabled(!connected);
    fDisconnectButton->SetEnabled(connected);
    fDrawButton->SetEnabled(connected);
    fClearButton->SetEnabled(connected);
    fClearAllButton->SetEnabled(connected);
    fStatusLabel->SetText(connected ? "Connected" : "Disconnected");
    Layout();
}

void TSupFDetMonGui::ConnectServer()
{
    if (fClient) fClient->Disconnect();

    const std::string host = fHostEntry->GetText();
    const int port = static_cast<int>(fPortEntry->GetNumber());

    fClient = std::make_unique<TSupFDetMonClient>(host, port);
    const bool connected = fClient->Connect();
    if (!connected) fClient.reset();
    SetConnectedUi(connected);
}

void TSupFDetMonGui::DisconnectServer()
{
    if (fClient) {
        fClient->Disconnect();
        fClient.reset();
    }
    SetConnectedUi(false);
}

void TSupFDetMonGui::SelectionChanged(Int_t)
{
    UpdateHistogramName();
}

void TSupFDetMonGui::DrawSelected()
{
    if (!fClient || !fClient->IsConnected()) return;

    fHistogram = fClient->GetHistogram(SelectedHistogramName());
    if (!fHistogram) return;

    TCanvas* canvas = fCanvas->GetCanvas();
    canvas->cd();
    canvas->Clear();
    fHistogram->Draw();
    canvas->Modified();
    canvas->Update();
}

void TSupFDetMonGui::ClearSelected()
{
    if (!fClient || !fClient->IsConnected()) return;
    if (fClient->ClearHistogram(SelectedHistogramName()))
        DrawSelected();
}

void TSupFDetMonGui::ClearAllHistograms()
{
    if (!fClient || !fClient->IsConnected()) return;
    if (fClient->ClearAll())
        DrawSelected();
}

void TSupFDetMonGui::CloseWindow()
{
    DisconnectServer();
    DeleteWindow();
}
