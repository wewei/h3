/*
 * Copyright 2017 Uber Technologies, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/** @file testH3UniEdge.c
 * @brief Tests functions for manipulating unidirectional edge H3Indexes
 *
 * usage: `testH3UniEdge`
 */

#include <stdlib.h>
#include "constants.h"
#include "h3Index.h"
#include "test.h"

// Fixtures
GeoCoord sfGeo = {0.659966917655, -2.1364398519396};

BEGIN_TESTS(h3UniEdge);

TEST(h3IndexesAreNeighbors) {
    H3Index sf = H3_EXPORT(geoToH3)(&sfGeo, 9);
    H3Index* ring = calloc(H3_EXPORT(maxKringSize)(1), sizeof(H3Index));
    H3_EXPORT(hexRing)(sf, 1, ring);

    t_assert(H3_EXPORT(h3IndexesAreNeighbors)(sf, sf) == 0,
             "an index does not neighbor itself");

    int neighbors = 0;
    for (int i = 0; i < H3_EXPORT(maxKringSize)(1); i++) {
        if (ring[i] != 0 && H3_EXPORT(h3IndexesAreNeighbors)(sf, ring[i])) {
            neighbors++;
        }
    }
    t_assert(neighbors == 6,
             "got the expected number of neighbors from a k-ring of 1");

    H3Index* largerRing = calloc(H3_EXPORT(maxKringSize)(2), sizeof(H3Index));
    H3_EXPORT(hexRing)(sf, 2, largerRing);

    neighbors = 0;
    for (int i = 0; i < H3_EXPORT(maxKringSize)(2); i++) {
        if (largerRing[i] != 0 &&
            H3_EXPORT(h3IndexesAreNeighbors)(sf, largerRing[i])) {
            neighbors++;
        }
    }
    t_assert(neighbors == 0,
             "got no neighbors, as expected, from a k-ring of 2");
    free(largerRing);

    H3Index sfBroken = sf;
    H3_SET_MODE(sfBroken, H3_UNIEDGE_MODE);
    t_assert(H3_EXPORT(h3IndexesAreNeighbors)(sf, sfBroken) == 0,
             "broken H3Indexes can't be neighbors");

    H3Index sfBigger = H3_EXPORT(geoToH3)(&sfGeo, 7);
    t_assert(H3_EXPORT(h3IndexesAreNeighbors)(sf, sfBigger) == 0,
             "hexagons of different resolution can't be neighbors");

    t_assert(H3_EXPORT(h3IndexesAreNeighbors)(ring[2], ring[1]) == 1,
             "hexagons in a ring are neighbors");
    free(ring);
}

TEST(getH3UnidirectionalEdgeAndFriends) {
    H3Index sf = H3_EXPORT(geoToH3)(&sfGeo, 9);
    H3Index* ring = calloc(H3_EXPORT(maxKringSize)(1), sizeof(H3Index));
    H3_EXPORT(hexRing)(sf, 1, ring);
    H3Index sf2 = ring[0];
    free(ring);

    H3Index edge = H3_EXPORT(getH3UnidirectionalEdge)(sf, sf2);
    t_assert(sf == H3_EXPORT(getOriginH3IndexFromUnidirectionalEdge)(edge),
             "can retrieve the origin from the edge");
    t_assert(
        sf2 == H3_EXPORT(getDestinationH3IndexFromUnidirectionalEdge)(edge),
        "can retrieve the destination from the edge");

    H3Index originDestination[2] = {0};
    H3_EXPORT(getH3IndexesFromUnidirectionalEdge)(edge, originDestination);
    t_assert(originDestination[0] == sf,
             "got the origin first in the pair request");
    t_assert(originDestination[1] == sf2,
             "got the destination last in the pair request");

    H3Index* largerRing = calloc(H3_EXPORT(maxKringSize)(2), sizeof(H3Index));
    H3_EXPORT(hexRing)(sf, 2, largerRing);
    H3Index sf3 = largerRing[0];
    free(largerRing);

    H3Index notEdge = H3_EXPORT(getH3UnidirectionalEdge)(sf, sf3);
    t_assert(notEdge == 0, "Non-neighbors can't have edges");
}

TEST(getH3UnidirectionalEdgeFromPentagon) {
    H3Index pentagon;
    setH3Index(&pentagon, 0, 4, 0);
    H3Index adjacent;
    setH3Index(&adjacent, 0, 8, 0);

    H3Index edge = H3_EXPORT(getH3UnidirectionalEdge)(pentagon, adjacent);
    t_assert(edge != 0, "Produces a valid edge");
}

TEST(h3UnidirectionalEdgeIsValid) {
    H3Index sf = H3_EXPORT(geoToH3)(&sfGeo, 9);
    H3Index* ring = calloc(H3_EXPORT(maxKringSize)(1), sizeof(H3Index));
    H3_EXPORT(hexRing)(sf, 1, ring);
    H3Index sf2 = ring[0];
    free(ring);

    H3Index edge = H3_EXPORT(getH3UnidirectionalEdge)(sf, sf2);
    t_assert(H3_EXPORT(h3UnidirectionalEdgeIsValid)(edge) == 1,
             "edges validate correctly");
    t_assert(H3_EXPORT(h3UnidirectionalEdgeIsValid)(sf) == 0,
             "hexagons do not validate");

    H3Index fakeEdge = sf;
    H3_SET_MODE(fakeEdge, H3_UNIEDGE_MODE);
    t_assert(H3_EXPORT(h3UnidirectionalEdgeIsValid)(fakeEdge) == 0,
             "edges without an edge specified don't work");

    H3Index pentagon = 0x821c07fffffffff;
    H3Index goodPentagonalEdge = pentagon;
    H3_SET_MODE(goodPentagonalEdge, H3_UNIEDGE_MODE);
    H3_SET_RESERVED_BITS(goodPentagonalEdge, 2);
    t_assert(H3_EXPORT(h3UnidirectionalEdgeIsValid)(goodPentagonalEdge) == 1,
             "pentagonal edge validates");

    H3Index badPentagonalEdge = goodPentagonalEdge;
    H3_SET_RESERVED_BITS(badPentagonalEdge, 1);
    t_assert(H3_EXPORT(h3UnidirectionalEdgeIsValid)(badPentagonalEdge) == 0,
             "missing pentagonal edge does not validate");
}

TEST(getH3UnidirectionalEdgesFromHexagon) {
    H3Index sf = H3_EXPORT(geoToH3)(&sfGeo, 9);
    H3Index* edges = calloc(6, sizeof(H3Index));
    H3_EXPORT(getH3UnidirectionalEdgesFromHexagon)(sf, edges);

    for (int i = 0; i < 6; i++) {
        t_assert(H3_EXPORT(h3UnidirectionalEdgeIsValid)(edges[i]) == 1,
                 "edge is an edge");
        t_assert(
            sf == H3_EXPORT(getOriginH3IndexFromUnidirectionalEdge)(edges[i]),
            "origin is correct");
        t_assert(sf != H3_EXPORT(getDestinationH3IndexFromUnidirectionalEdge)(
                           edges[i]),
                 "destination is not origin");
    }
    free(edges);

    H3Index pentagon = 0x821c07fffffffff;
    edges = calloc(6, sizeof(H3Index));
    H3_EXPORT(getH3UnidirectionalEdgesFromHexagon)(pentagon, edges);

    int missingEdgeCount = 0;
    for (int i = 0; i < 6; i++) {
        if (edges[i] == 0) {
            missingEdgeCount++;
        } else {
            t_assert(H3_EXPORT(h3UnidirectionalEdgeIsValid)(edges[i]) == 1,
                     "edge is an edge");
            t_assert(
                pentagon ==
                    H3_EXPORT(getOriginH3IndexFromUnidirectionalEdge)(edges[i]),
                "origin is correct");
            t_assert(
                sf != H3_EXPORT(getDestinationH3IndexFromUnidirectionalEdge)(
                          edges[i]),
                "destination is not origin");
        }
    }
    t_assert(missingEdgeCount == 1,
             "Only one edge was deleted for the pentagon");
    free(edges);
}

TEST(getH3UnidirectionalEdgeBoundary) {
    H3Index sf = H3_EXPORT(geoToH3)(&sfGeo, 9);
    H3Index* edges = calloc(6, sizeof(H3Index));
    H3_EXPORT(getH3UnidirectionalEdgesFromHexagon)(sf, edges);

    GeoBoundary gb;
    for (int i = 0; i < 6; i++) {
        H3_EXPORT(getH3UnidirectionalEdgeBoundary)(edges[i], &gb);
        t_assert(gb.numVerts == 2, "Got the expected number of vertices back");
    }
    free(edges);

    H3Index pentagon = 0x811c0ffffffffff;
    edges = calloc(6, sizeof(H3Index));
    H3_EXPORT(getH3UnidirectionalEdgesFromHexagon)(pentagon, edges);

    int missingEdgeCount = 0;
    for (int i = 0; i < 6; i++) {
        if (edges[i] == 0) {
            missingEdgeCount++;
        } else {
            H3_EXPORT(getH3UnidirectionalEdgeBoundary)(edges[i], &gb);
            t_assert(gb.numVerts == 3,
                     "Got the expected number of vertices back for a Class III "
                     "pentagon");
        }
    }
    t_assert(missingEdgeCount == 1,
             "Only one edge was deleted for the pentagon");
    free(edges);
}

END_TESTS();
