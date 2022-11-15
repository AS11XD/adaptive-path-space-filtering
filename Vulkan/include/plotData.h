#pragma once
#include <assert.h>
#include <vector>

template <typename T>
class PlotData
{
private:
	std::vector<T> data = std::vector<T>(1);
	size_t current_pointer = 0;
	size_t max_size = 0;
	bool full = false;
	float average = 0.0f;

public:
	PlotData(size_t _max_size = 1000);

	void appendData(T d);
	T getNewestData();
	float averageData();
	float averageData(size_t over);
	float oldAverageData();
	T* getDataPointer();
	size_t getCurrentSize();
	size_t getMaxSize();
	bool getFull();
	void clear();

	PlotData<T> operator+(const PlotData<T>& plot);
};

template<typename T>
inline PlotData<T>::PlotData(size_t _max_size) : max_size(_max_size)
{
	data.resize(_max_size);
}

template <typename T>
inline void PlotData<T>::appendData(T d)
{
	current_pointer++;
	if (current_pointer == data.size())
	{
		current_pointer = 0;
		full = true;
	}
	data[current_pointer] = d;
}

template <typename T>
inline T PlotData<T>::getNewestData()
{
	return data[current_pointer];
}

template <typename T>
inline float PlotData<T>::averageData(size_t over)
{
	size_t _over = (full) ? std::min(over, data.size()) : std::min(over, current_pointer + 1);

	float sum = 0.0f;
	for (size_t i = 0; i < _over; ++i)
	{
		sum += (float)data[(current_pointer - i) % data.size()];
	}
	average = sum / _over;
	return average;
}

template<typename T>
inline float PlotData<T>::oldAverageData()
{
	return average;
}

template <typename T>
inline float PlotData<T>::averageData()
{
	if (full)
	{
		float sum = 0.0f;
		for (T& s : data)
		{
			sum += (float)s;
		}
		average = sum / data.size();
		return average;
	}
	else
	{
		float sum = 0.0f;
		for (size_t i = 0; i <= current_pointer; ++i)
		{
			sum += (float)data[i];
		}
		average = sum / (float)current_pointer;
		return average;
	}
}

template <typename T>
inline T* PlotData<T>::getDataPointer()
{
	return data.data();
}

template<typename T>
inline size_t PlotData<T>::getCurrentSize()
{
	return (full) ? data.size() : current_pointer;
}

template<typename T>
inline size_t PlotData<T>::getMaxSize()
{
	return data.size();
}

template<typename T>
inline bool PlotData<T>::getFull()
{
	return full;
}

template<typename T>
inline void PlotData<T>::clear()
{
	data.clear();
	data.resize(max_size);
	current_pointer = 0;
	full = false;
	average = 0.0f;
}

template<typename T>
inline PlotData<T> PlotData<T>::operator+(const PlotData<T>& plot)
{
	assert(data.size() == plot.data.size());
	assert(current_pointer == plot.current_pointer);
	assert(full == plot.full);
	PlotData result(data.size());
	result.current_pointer = current_pointer;
	result.full = full;
	for (size_t i = 0; i < data.size(); ++i)
	{
		result.data[i] = data[i] + plot.data[i];
	}
	return result;
}
