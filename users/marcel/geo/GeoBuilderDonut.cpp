#include "GeoBuilder.h"
#include "Math.h"

namespace Geo
{

namespace Builder
{

/// Geometry: Primitives: Donut generator.
/**
 * Generates a donut around the z axis, with an inner radius of r1 and an outer radius of r2.
 * The higher the number of slices and sides, the better the approximation of a true donut will be.
 * The generated polygons will be stored in the specified mesh.
 * @param mesh Mesh to put generated polygon in.
 * @param sides Number of divisions around z axis. The more the smoother.
 * @param slices Number of divisions around local y axis. The more the smoother.
 * @param r1 Inner radius.
 * @param r2 Outer radius.
 */
void Geo::Builder::MakeDonut(Mesh& mesh, int sides, int slices, float r1, float r2)
{

	r1 += r2;
        
	float s1 = 2.0f * M_PI / sides;
    float a1 = 0.0f;

    for (int i = 0; i < sides; ++i)
	{

        float s2 = 2.0f * M_PI / slices;
        float a2 = 0.0f;

        for (int j = 0; j < slices; ++j)
        {

            Poly* poly = mesh.Add();
                        
            matrixStack.Push();
            matrixStack.GetMatrix().MakeIdentity();
              
            Vec3 r, t;
                              
            matrixStack.Push();
            
            r[0] = 0.0f;
            r[1] = 0.0f;
            r[2] = - a1;
            
            t[0] = r1;
            t[1] = 0.0f;
            t[2] = 0.0f;
            
            matrixStack.ApplyRotationEuler(r);
            matrixStack.ApplyTranslation(t);
            
            Vertex* vertex1 = poly->Add(); vertex1->position.Set(sinf(a2 + s2) * r2, 0.0f, cosf(a2 + s2) * r2);
            Vertex* vertex2 = poly->Add(); vertex2->position.Set(sinf(a2     ) * r2, 0.0f, cosf(a2     ) * r2);
            
            vertex1->position = matrixStack.GetMatrix() * vertex1->position;
            vertex2->position = matrixStack.GetMatrix() * vertex2->position;
            
            matrixStack.Pop();

            matrixStack.Push();
            
            r[0] = 0.0f;
            r[1] = 0.0f;
            r[2] = - a1 - s1;
            
            t[0] = r1;
            t[1] = 0.0f;
            t[2] = 0.0f;
            
            matrixStack.ApplyRotationEuler(r);
            matrixStack.ApplyTranslation(t);
            
            Vertex* vertex3 = poly->Add(); vertex3->position.Set(sinf(a2     ) * r2, 0.0f, cosf(a2     ) * r2);
            Vertex* vertex4 = poly->Add(); vertex4->position.Set(sinf(a2 + s2) * r2, 0.0f, cosf(a2 + s2) * r2);
            
            vertex3->position = matrixStack.GetMatrix() * vertex3->position;
            vertex4->position = matrixStack.GetMatrix() * vertex4->position;
            
            matrixStack.Pop();

			matrixStack.Pop();
			
			Transform(poly);
                        
            a2 += s2;
                
        }

        a1 += s1;

    }

}

}; // Builder.

}; // Geo.
