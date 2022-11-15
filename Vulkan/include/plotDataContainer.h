#pragma once
#include <vector>
#include <string>
#include <initializer_list>
#include "plotData.h"

template <typename T>
class PlotDataContainer
{
public:
	std::vector<std::pair<std::string, PlotData<T>>> container;
	PlotDataContainer(std::initializer_list<std::pair<std::string, PlotData<T>>> l);
	PlotDataContainer(PlotDataContainer& pdc);
	PlotDataContainer();

	void reset();
	PlotData<T>& operator [](std::string idx);
};

template<typename T>
inline PlotDataContainer<T>::PlotDataContainer(std::initializer_list<std::pair<std::string, PlotData<T>>> l) : container(l)
{

}

template<typename T>
inline PlotDataContainer<T>::PlotDataContainer(PlotDataContainer& pdc) : container(pdc.container)
{

}

template<typename T>
inline PlotDataContainer<T>::PlotDataContainer()
{

}

template<typename T>
inline void PlotDataContainer<T>::reset()
{
	for (auto& c : container)
	{
		c.second.clear();
	}
}

template<typename T>
inline PlotData<T>& PlotDataContainer<T>::operator[](std::string idx)
{
	for (auto& i : container)
	{
		if (i.first.compare(idx) == 0)
			return i.second;
	}
	return PlotData<T>();
}
