/* \author Aaron Brown */
// Create simple 3d highway enviroment using PCL
// for exploring self-driving car sensors

#include "sensors/lidar.h"
#include "render/render.h"
#include "processPointClouds.h"
// using templates for processPointClouds so also include .cpp to help linker
#include "processPointClouds.cpp"

Lidar *gpLidar;

std::vector<Car> initHighway(bool renderScene, pcl::visualization::PCLVisualizer::Ptr& viewer)
{

    Car egoCar( Vect3(0,0,0), Vect3(4,2,2), Color(0,1,0), "egoCar");
    Car car1( Vect3(15,0,0), Vect3(4,2,2), Color(0,0,1), "car1");
    Car car2( Vect3(8,-4,0), Vect3(4,2,2), Color(0,0,1), "car2");	
    Car car3( Vect3(-12,4,0), Vect3(4,2,2), Color(0,0,1), "car3");
  
    std::vector<Car> cars;
    cars.push_back(egoCar);
    cars.push_back(car1);
    cars.push_back(car2);
    cars.push_back(car3);

    if(renderScene)
    {
        renderHighway(viewer);
        egoCar.render(viewer);
        car1.render(viewer);
        car2.render(viewer);
        car3.render(viewer);
    }

    return cars;
}


void simpleHighway(pcl::visualization::PCLVisualizer::Ptr& viewer)
{
    // ----------------------------------------------------
    // -----Open 3D viewer and display simple highway -----
    // ----------------------------------------------------
    
    // RENDER OPTIONS
    bool renderScene = false;
    std::vector<Car> cars = initHighway(renderScene, viewer);

    
    // TODO:: Create lidar sensor
    gpLidar = new Lidar(cars, 0.0);
    const typename pcl::PointCloud<pcl::PointXYZ>::Ptr pcd{ gpLidar->scan() };
    //renderRays(viewer, gpLidar->position, pcd);
    //renderPointCloud(viewer, pcd, "Point cloud");


    // TODO:: Create point processor
    ProcessPointClouds<pcl::PointXYZ> pointProcessor;
    auto segPairs = pointProcessor.SegmentPlane(pcd, 100, 0.5);
//    renderPointCloud(viewer, segPairs.first, "obstacles", Color(1, 0, 0));
//    renderPointCloud(viewer, segPairs.second, "plane", Color(0, 1, 0));

    std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr> cloudClusters = pointProcessor.Clustering(segPairs.first, 1.0, 3, 30);

    int clusterId = 0;
    std::vector<Color> colors = {Color(1,0,0), Color(1,1,0), Color(0,0,1)};

    for(pcl::PointCloud<pcl::PointXYZ>::Ptr cluster : cloudClusters)
    {
          std::cout << "cluster size ";
          pointProcessor.numPoints(cluster);
          renderPointCloud(viewer, cluster, "obstCloud"+std::to_string(clusterId), colors[clusterId]);

          Box boundingBox = pointProcessor.BoundingBox(cluster);
          renderBox(viewer, boundingBox, clusterId, colors[clusterId]);

          ++clusterId;
    }
}


void cityBlock(pcl::visualization::PCLVisualizer::Ptr& viewer,
               ProcessPointClouds<pcl::PointXYZI> &pointProcessorI,
               typename pcl::PointCloud<pcl::PointXYZI>::Ptr cloud)
{
    // Filtration
    Eigen::Vector4f minLeaf {-10.0, -7.0, -2.0, 1.0};
    Eigen::Vector4f maxLeaf { 20.0,  7.0,  5.0, 1.0};
    cloud = pointProcessorI.FilterCloud(cloud, 0.2, minLeaf, maxLeaf);
    //renderPointCloud(viewer, cloud, "City Block");

    // Segmentation
    auto segPairs = pointProcessorI.SegmentPlane(cloud, 50, 0.15);
//    renderPointCloud(viewer, segPairs.first, "obstacles", Color(1, 0, 0));
    renderPointCloud(viewer, segPairs.second, "road", Color(0, 1, 0));


    // Clustering
    int clusterId = 0;
    std::vector<pcl::PointCloud<pcl::PointXYZI>::Ptr> clusters = pointProcessorI.Clustering(segPairs.first, 0.5, 5, 1000);
    std::vector<Color> colors = {Color(1,0,0), Color(1,1,0), Color(0,0,1)};

    for (auto cluster : clusters)
    {
        std::cout << "cluster size ";
        pointProcessorI.numPoints(cluster);

        int colorID = clusterId % colors.size();
        renderPointCloud(viewer, cluster, "obstCloud"+std::to_string(clusterId), colors[colorID]);

        Box boundingBox = pointProcessorI.BoundingBox(cluster);
        renderBox(viewer, boundingBox, clusterId);

        clusterId++;
    }
}

//setAngle: SWITCH CAMERA ANGLE {XY, TopDown, Side, FPS}
void initCamera(CameraAngle setAngle, pcl::visualization::PCLVisualizer::Ptr& viewer)
{
    viewer->setBackgroundColor (0, 0, 0);
    
    // set camera position and angle
    viewer->initCameraParameters();
    // distance away in meters
    int distance = 16;
    
    switch(setAngle)
    {
        case XY : viewer->setCameraPosition(-distance, -distance, distance, 1, 1, 0); break;
        case TopDown : viewer->setCameraPosition(0, 0, distance, 1, 0, 1); break;
        case Side : viewer->setCameraPosition(0, -distance, 0, 0, 0, 1); break;
        case FPS : viewer->setCameraPosition(-10, 0, 0, 0, 0, 1);
    }

    if(setAngle!=FPS)
        viewer->addCoordinateSystem (1.0);
}


int main (int argc, char** argv)
{
    std::cout << "starting enviroment" << std::endl;

    pcl::visualization::PCLVisualizer::Ptr viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));
    CameraAngle setAngle = XY;
    initCamera(setAngle, viewer);
    //simpleHighway(viewer);
    //cityBlock(viewer);

    ProcessPointClouds<pcl::PointXYZI> pointProcessorI;
    std::vector<boost::filesystem::path> stream = pointProcessorI.streamPcd("../src/sensors/data/pcd/data_1");
    auto streamIterator = stream.begin();
    pcl::PointCloud<pcl::PointXYZI>::Ptr inputCloudI;

    while (!viewer->wasStopped ())
    {
        viewer->removeAllPointClouds();
        viewer->removeAllShapes();

        inputCloudI = pointProcessorI.loadPcd( (*streamIterator).string() );
        cityBlock(viewer, pointProcessorI, inputCloudI);

        streamIterator++;
        if(streamIterator == stream.end())
        {
            streamIterator = stream.begin();
        }

        viewer->spinOnce ();
    }

    delete gpLidar;
}
