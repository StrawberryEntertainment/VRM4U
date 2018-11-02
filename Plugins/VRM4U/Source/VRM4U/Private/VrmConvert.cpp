// Fill out your copyright notice in the Description page of Project Settings.

#include "VrmConvert.h"


static bool bImportMode = false;
bool VRM::IsImportMode() {
	return bImportMode;
}
void VRM::SetImportMode(bool b) {
	bImportMode = b;
}


VrmConvert::VrmConvert()
{
}

VrmConvert::~VrmConvert()
{
}

