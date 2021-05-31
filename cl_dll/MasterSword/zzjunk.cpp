/
//  Mirrror all vertices of the level
//
//  ===============================


				//Keep a copy of the mirrored level
				/*pUseMirror->WorldSurfaces.reserve( pModel->numsurfaces );
				 for (int r = 0; r < pModel->numsurfaces; r++) 
				{
					msurface_t &WorldSurface = pModel->surfaces[r];
					glpoly_t *Poly = WorldSurface.polys;
					if( Poly->next || Poly->chain )
						int stop = 0;

					float *v = Poly->verts[0];

					//Create new polygon info
					//Create extra data because the vertices will go beyond the end of this struct.
					//This allocation is actually too big, because some verts are stored at the end of glpoly_t,
					//but I create a tiny bit extra, just to be sure.
					glpoly_t *NewPoly = (glpoly_t *)new byte[sizeof(glpoly_t) + sizeof(float) * VERTEXSIZE * Poly->numverts];
					*NewPoly = *Poly;					//Copy old poly to new poly

					//if( Poly->numverts >= 15 )
					//	MSErrorConsoleText( "SetupLevel", "Error: Found surfaces with >= 15 vertices" );

					//Flip vertices across normal and
					//Reverse vertex order
					for( int t = 0; t < Poly->numverts; t++, v += VERTEXSIZE )
						memcpy( BigVerts[t], v, sizeof(float) * VERTEXSIZE );

					v = NewPoly->verts[0];
					//for( int t = 0; t < Poly->numverts; t++, v += VERTEXSIZE )
					for( int t = Poly->numverts-1; t >= 0; t--, v+= VERTEXSIZE )
					{
						memcpy( v, BigVerts[t], sizeof(float) * VERTEXSIZE );
						Vector &Vert = *(Vector *)v;
						 for (int i = 0; i < 3; i++) 
							Vert[i] += Vert[i] * -pUseMirror->Normal[i] * 2 + pUseMirror->Normal[i] * pUseMirror->Dist;
					}

					msurface_t &NewSurface = pUseMirror->WorldSurfaces.add( WorldSurface );
					NewSurface.polys = NewPoly;
				}*/

				/*pUseMirror->MarkSurfaces = new msurface_t *[pModel->nummarksurfaces * 2];
				pUseMirror->Leafs.reserve( pModel->numleafs * 2 );
				pUseMirror->Nodes.reserve( pModel->numnodes );

				TraverseInfo_t Info;
				Info.pParent = NULL;
				Info.ChildNum = 0;
				Info.pNode = pModel->nodes;
				Info.pUseMirror = pUseMirror;
				Info.pModel = pModel;
				TraverseNodes( Info );*/

				//pModel->nodes = pUseMirror->Nodes.FirstItem();
				//pModel->surfaces = pUseMirror->WorldSurfaces.FirstItem();
				//pModel->marksurfaces = pUseMirror->MarkSurfaces;
				//pModel->leafs = pUseMirror->Leafs.FirstItem();


//
//  FIND Surface Midpoint
//
//  =====================

			/*Vector Bounds[2];
			foreach( c, 3 ) { Bounds[0][c] = 99999999.0f; Bounds[1][c] = -99999999.0f; }

			 for (int e = 0; e < Surface.numedges; e++) 
			{
				int iEdge = pModel->surfedges[Surface.firstedge + e];
				if( iEdge  < 0 ) iEdge = -iEdge;

				 for (int p = 0; p < 2; p++) 
				{
					mvertex_t &Vert = pModel->vertexes[pModel->edges[iEdge].v[p]];
					 for (int c = 0; c < 3; c++) 
					{
						if( Vert.position[c] < Bounds[0][c] ) Bounds[0][c] = Vert.position[c];
						if( Vert.position[c] > Bounds[1][c] ) Bounds[1][c] = Vert.position[c];
					}
				}
			}
			
			Vector Length = Bounds[1] - Bounds[0];
			Vector Mid = Bounds[0] + (Bounds[1] - Bounds[0]) / 2.0f;*/