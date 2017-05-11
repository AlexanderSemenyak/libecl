#!/usr/bin/env python
#  Copyright (C) 2017  Statoil ASA, Norway. 
#   
#  The file 'test_grid_generator.py' is part of ERT - Ensemble based Reservoir Tool.
#   
#  ERT is free software: you can redistribute it and/or modify 
#  it under the terms of the GNU General Public License as published by 
#  the Free Software Foundation, either version 3 of the License, or 
#  (at your option) any later version. 
#   
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#  FITNESS FOR A PARTICULAR PURPOSE.   
#   
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
#  for more details.

from ecl.ecl import EclGrid
from ecl.ecl import EclGridGenerator as GridGen
from ecl.test import ExtendedTestCase, TestAreaContext
from itertools import product as prod
import operator

def generate_ijk_bounds(dims):
    ibounds = [(l, u) for l in range(dims[0]) for u in range(l, dims[0])]
    jbounds = [(l, u) for l in range(dims[1]) for u in range(l, dims[1])]
    kbounds = [(l, u) for l in range(dims[2]) for u in range(l, dims[2])]
    return prod(ibounds, jbounds, kbounds)

def decomposition_preserving(ijk_bound):
    return sum(zip(*ijk_bound)[0])%2 is 0

class GridGeneratorTest(ExtendedTestCase):

    def setUp(self):
        self.test_base = [
            (
                list(GridGen.createCoord((4,4,4), (1,1,1))),
                list(GridGen.createZcorn((4,4,4), (1,1,1), offset=0)),
                GridGen.createGrid((4,4,4), (1,1,1), offset=0)
            ),
            (
                list(
                    GridGen.createCoord((5,5,5), (1,1,1),
                    translation=(10,10,0))
                    ),
                list(
                    GridGen.createZcorn((5,5,5), (1,1,1), offset=0.5,
                    irregular_offset=True, concave=True, irregular=True)
                    ),
                GridGen.createGrid(
                    (5,5,5), (1,1,1), offset=0.5,
                    irregular=True, irregular_offset=True, concave=True,
                    translation=(10,10,0)
                    )
            )
            ]

    def test_extract_grid_decomposition_change(self):
        dims = (4,4,4)
        zcorn = list(GridGen.createZcorn(dims, (1,1,1), offset=0))
        coord = list(GridGen.createCoord(dims, (1,1,1)))

        ijk_bounds = generate_ijk_bounds(dims)
        for ijk_bounds in ijk_bounds:
            if decomposition_preserving(ijk_bounds):
                GridGen.extract_grid(dims, coord, zcorn, ijk_bounds)
            else:
                with self.assertRaises(ValueError):
                    GridGen.extract_grid(dims, coord, zcorn, ijk_bounds)

            GridGen.extract_grid(dims,
                                 coord, zcorn,
                                 ijk_bounds,
                                 decomposition_change=True)

    def test_extract_grid_invalid_bounds(self):
        dims = (3,3,3)
        zcorn = list(GridGen.createZcorn(dims, (1,1,1), offset=0))
        coord = list(GridGen.createCoord(dims, (1,1,1)))

        with self.assertRaises(ValueError):
            GridGen.extract_grid(dims, coord, zcorn, ((-1,0), (2,2), (2,2)))

        with self.assertRaises(ValueError):
            GridGen.extract_grid(dims, coord, zcorn, ((1,6), (2,2), (2,2)))
                
        with self.assertRaises(ValueError):
            GridGen.extract_grid(dims, coord, zcorn, ((1,2), (2,0), (2,2)))

    def test_extract_grid_slice_spec(self):
        dims = (4,4,4)
        zcorn = list(GridGen.createZcorn(dims, (1,1,1), offset=0))
        coord = list(GridGen.createCoord(dims, (1,1,1)))

        ijk_bounds = generate_ijk_bounds(dims)
        for ijk in ijk_bounds:
            ijk = list(ijk)
            for i in range(3):
                if len(set(ijk[i])) == 1:
                    ijk[i] = ijk[i][0]

            GridGen.extract_grid(dims,
                                 coord, zcorn,
                                 ijk,
                                 decomposition_change=True)

    def assertSubgrid(self, grid, subgrid, ijk_bound):
        sijk_space = prod(*[range(d) for d in subgrid.getDims()[:-1:]])
        for sijk in sijk_space:
            gijk = tuple([a+b for a, b in zip(sijk, zip(*ijk_bound)[0])])
            self.assertEqual(
                [subgrid.getCellCorner(i, ijk=sijk) for i in range(8)],
                [grid.getCellCorner(i, ijk=gijk) for i in range(8)]
                )

    def test_validate_cells(self):
        for coord, zcorn, grid in self.test_base:
            grid_dims = grid.getDims()[:-1:]
            ijk_bounds = generate_ijk_bounds(grid_dims)
            for ijk_bound in ijk_bounds:
                if not decomposition_preserving(ijk_bound):
                    continue

                sub_dims = tuple([u-l+1 for l, u in ijk_bound])
                sub_coord, sub_zcorn = GridGen.extract_grid(
                                                    grid_dims,
                                                    coord,
                                                    zcorn,
                                                    ijk_bound
                                                    )

                subgrid = EclGrid.create(sub_dims, sub_zcorn, sub_coord, None)
                self.assertEqual(sub_dims, subgrid.getDims()[:-1:])
                self.assertSubgrid(grid, subgrid, ijk_bound)
