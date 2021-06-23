#pragma once
#include "Properties.h"

struct PropertyWrapper : PropertyStruct
{
    PropertyWrapper()
    {
    }
    
    PropertyWrapper(const PropertyStruct &base)
        : PropertyStruct(base) 
    {
        
    }
    
    bool Initialized = false;
};

extern PropertyWrapper g_properties;
