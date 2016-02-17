// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LEETDEMO2.h"
#include "LEETDEMO2GameMode.h"
#include "LEETDEMO2Character.h"

ALEETDEMO2GameMode::ALEETDEMO2GameMode()
{
	// set default pawn class to our character
	DefaultPawnClass = ALEETDEMO2Character::StaticClass();	
}
