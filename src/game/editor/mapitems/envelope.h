#ifndef GAME_EDITOR_MAPITEMS_ENVELOPE_H
#define GAME_EDITOR_MAPITEMS_ENVELOPE_H

#include <game/editor/editor.h>

#include <chrono>

using namespace std::chrono_literals;

class CEnvelope
{
	class CEnvelopePointAccess : public IEnvelopePointAccess
	{
		std::vector<CEnvPoint_runtime> *m_pvPoints;

	public:
		CEnvelopePointAccess(std::vector<CEnvPoint_runtime> *pvPoints)
		{
			m_pvPoints = pvPoints;
		}

		int NumPoints() const override
		{
			return m_pvPoints->size();
		}

		const CEnvPoint *GetPoint(int Index) const override
		{
			if(Index < 0 || (size_t)Index >= m_pvPoints->size())
				return nullptr;
			return &m_pvPoints->at(Index);
		}

		const CEnvPointBezier *GetBezier(int Index) const override
		{
			if(Index < 0 || (size_t)Index >= m_pvPoints->size())
				return nullptr;
			return &m_pvPoints->at(Index).m_Bezier;
		}
	};

	int m_Channels;

public:
	std::vector<CEnvPoint_runtime> m_vPoints;
	CEnvelopePointAccess m_PointsAccess;
	char m_aName[32];
	float m_Bottom, m_Top;
	bool m_Synchronized;

	CEnvelope(int Channels) :
		m_PointsAccess(&m_vPoints)
	{
		SetChannels(Channels);
		m_aName[0] = '\0';
		m_Bottom = 0;
		m_Top = 0;
		m_Synchronized = false;
	}

	void Resort()
	{
		std::sort(m_vPoints.begin(), m_vPoints.end());
		FindTopBottom(0xf);
	}

	void FindTopBottom(int ChannelMask)
	{
		m_Top = -1000000000.0f;
		m_Bottom = 1000000000.0f;
		CEnvPoint_runtime *pPrevPoint = nullptr;
		for(auto &Point : m_vPoints)
		{
			for(int c = 0; c < m_Channels; c++)
			{
				if(ChannelMask & (1 << c))
				{
					{
						// value handle
						const float v = fx2f(Point.m_aValues[c]);
						m_Top = maximum(m_Top, v);
						m_Bottom = minimum(m_Bottom, v);
					}

					if(Point.m_Curvetype == CURVETYPE_BEZIER)
					{
						// out-tangent handle
						const float v = fx2f(Point.m_aValues[c] + Point.m_Bezier.m_aOutTangentDeltaY[c]);
						m_Top = maximum(m_Top, v);
						m_Bottom = minimum(m_Bottom, v);
					}

					if(pPrevPoint != nullptr && pPrevPoint->m_Curvetype == CURVETYPE_BEZIER)
					{
						// in-tangent handle
						const float v = fx2f(Point.m_aValues[c] + Point.m_Bezier.m_aInTangentDeltaY[c]);
						m_Top = maximum(m_Top, v);
						m_Bottom = minimum(m_Bottom, v);
					}
				}
			}
			pPrevPoint = &Point;
		}
	}

	int Eval(float Time, ColorRGBA &Color)
	{
		CRenderTools::RenderEvalEnvelope(&m_PointsAccess, m_Channels, std::chrono::nanoseconds((int64_t)((double)Time * (double)std::chrono::nanoseconds(1s).count())), Color);
		return m_Channels;
	}

	void AddPoint(int Time, int v0, int v1 = 0, int v2 = 0, int v3 = 0)
	{
		CEnvPoint_runtime p;
		p.m_Time = Time;
		p.m_aValues[0] = v0;
		p.m_aValues[1] = v1;
		p.m_aValues[2] = v2;
		p.m_aValues[3] = v3;
		p.m_Curvetype = CURVETYPE_LINEAR;
		for(int c = 0; c < CEnvPoint::MAX_CHANNELS; c++)
		{
			p.m_Bezier.m_aInTangentDeltaX[c] = 0;
			p.m_Bezier.m_aInTangentDeltaY[c] = 0;
			p.m_Bezier.m_aOutTangentDeltaX[c] = 0;
			p.m_Bezier.m_aOutTangentDeltaY[c] = 0;
		}
		m_vPoints.push_back(p);
		Resort();
	}

	float EndTime() const
	{
		if(m_vPoints.empty())
			return 0.0f;
		return m_vPoints.back().m_Time / 1000.0f;
	}

	int GetChannels() const
	{
		return m_Channels;
	}

	void SetChannels(int Channels)
	{
		m_Channels = clamp<int>(Channels, 1, CEnvPoint::MAX_CHANNELS);
	}
};

#endif
