Mesh.MshFileVersion = 2.2;
Mesh.CharacteristicLengthExtendFromBoundary=1;
Mesh.CharacteristicLengthFromPoints=1;
Mesh.ElementOrder=1;
Mesh.SecondOrderIncomplete = 0;
Mesh.Algorithm = 6;
Mesh.Algorithm3D = 1;
//Mesh.OptimizeNetgen=1;
// partitioning data
Mesh.Partitioner=1;
Mesh.NbPartitions=1;
Mesh.MshFilePartitioned=0;

h=0.1;
Mesh.CharacteristicLengthMax=h;

Point(1) = {0, 0, 0, h};
a=0.785398163397448; // pi/4
Point(2) = {a, a, 0, h};
Point(3) = {2*a, 2*a, 0, h };
Point(4) = {1.5*a, 1.5*a, 0, h };
radius = Sqrt(2*0.5*a*0.5*a);
Point(5) = {1.5*a, 1.5*a, radius, h };
Point(6) = {1.5*a, 1.5*a, -radius, h };
Circle(1) = {2, 4, 6};
Circle(2) = {6, 4, 3};
Circle(3) = {3, 4, 5};
Circle(4) = {5, 4, 2};
Line Loop(5) = {3, 4, 1, 2};
Plane Surface(6) = {5};

Extrude {{0, 0, 1}, {0, 0, 0}, Pi/2} {
  Surface{6};
}
Surface Loop(29) = {27, 6, 15, 19, 23, 28};
Volume(30) = {29};
Physical Surface("inlet") = {28};
Physical Surface("outlet") = {6};
Physical Surface("wall") = {15, 19, 23, 27};
Physical Volume("Omega") = {30};