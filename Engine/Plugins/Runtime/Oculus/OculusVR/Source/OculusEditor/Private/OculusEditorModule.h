// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IOculusEditorModule.h"
#include "Modules/ModuleInterface.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"

class FToolBarBuilder;
class FMenuBuilder;

#define OCULUS_EDITOR_MODULE_NAME "OculusEditor"

enum class ECheckBoxState : uint8;

class FOculusEditorModule : public IOculusEditorModule
{
public:
	FOculusEditorModule() : bModuleValid(false) {};

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual void PostLoadCallback() override;

	void RegisterSettings();
	void UnregisterSettings();

	void PluginButtonClicked();
	FReply PluginClickFn(bool text);

	void PluginOpenPlatWindow();

	void ToggleOpenXRRuntime();

	void CreateSESSubMenus(FMenuBuilder& MenuBuilder);
	void LaunchSESGameRoom();
	void LaunchSESLivingRoom();
	void LaunchSESBedroom();
	void StopSESServer();

public:
	static const FName OculusPerfTabName;
	static const FName OculusPlatToolTabName;

private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	TSharedRef<SWidget> CreateToolbarEntryMenu(TSharedPtr<class FUICommandList> Commands);
	TSharedRef<SWidget> CreateXrSimToolbarEntryMenu(TSharedPtr<class FUICommandList> Commands);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<class SDockTab> OnSpawnPlatToolTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
	bool bModuleValid;
};

class IDetailLayoutBuilder;

class FOculusHMDSettingsDetailsCustomization : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface
	EVisibility GetContextualPassthroughWarningVisibility() const;
	FReply PluginClickPerfFn(bool text);
	FReply PluginClickPlatFn(bool text);
	FReply DisableEngineSplash(bool text);
};
