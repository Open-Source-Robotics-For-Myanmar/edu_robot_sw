# rom_dabai_3d_camera

> ROM Robotics — Dabai DW 3D Depth Camera Launch Package (ROS 2 Humble)

---

## Environment Variables

| Variable | Default | Description |
|---|---|---|
| `ROM_ROBOT_NAMESPACE` | `default_robot1` | ROS 2 namespace — topics/services အားလုံး `/<namespace>/...` အောက်တွင် ရှိမည် |
| `ROM_ROBOT_MODEL` | `edu_robot` | Robot model name (`rom_pcl_merge` param) |

```bash
# Set before launch
export ROM_ROBOT_NAMESPACE=edu_robot01
export ROM_ROBOT_MODEL=edu_robot
ros2 launch rom_dabai_3d_camera pointcloud_mini_dabai_dabai.launch.py use_rviz:=false
```

---

## Launch File Hierarchy

```
launch/
├── pointcloud_mini_dabai_dabai.launch.py   # Main entry (namespace-aware)
├── multi_dabai_dw.launch.xml               # Multi camera manager (included)
└── dabai_dw.launch.xml                     # Single camera node (included)
```

```mermaid
graph TD
    ENV["ENV: ROM_ROBOT_NAMESPACE\n(default: default_robot1)"]
    MAIN["pointcloud_mini_dabai_dabai.launch.py\n(Main Entry)"]
    NS["GroupAction + PushRosNamespace"]
    MULTI["multi_dabai_dw.launch.xml\n(Multi Camera Manager)"]
    P2L["pointcloud_to_laserscan_node\n(inline — relative topics)"]

    DAB1["dabai_dw.launch.xml\n(camera1)"]
    DAB2["dabai_dw.launch.xml\n(camera2)"]

    ENV --> MAIN --> NS
    NS --> MULTI
    NS --> P2L
    MULTI -- "include\ncamera1_serial" --> DAB1
    MULTI -- "include\ncamera2_serial" --> DAB2

    style ENV fill:#FFF9C4,stroke:#F57F17,color:#000
    style MAIN fill:#E8EAF6,stroke:#283593,color:#000
    style NS fill:#FFCDD2,stroke:#B71C1C,color:#000
    style MULTI fill:#BBDEFB,stroke:#1565C0,color:#000
    style P2L fill:#FFE0B2,stroke:#E65100,color:#000
    style DAB1 fill:#C8E6C9,stroke:#2E7D32,color:#000
    style DAB2 fill:#C8E6C9,stroke:#2E7D32,color:#000
```

---

## pointcloud_mini_dabai_dabai.launch.py — Nodes & Connections

```mermaid
graph TB
    subgraph ns["GroupAction + PushRosNamespace(ROM_ROBOT_NAMESPACE)"]

        subgraph "include: multi_dabai_dw.launch.xml"
            subgraph "include: dabai_dw.launch.xml (camera1)"
                CAM1["astra_camera_node\npkg: astra_camera\nns: camera1\nserial: CH1K73100C5"]
            end
            subgraph "include: dabai_dw.launch.xml (camera2)"
                CAM2["astra_camera_node\npkg: astra_camera\nns: camera2\nserial: CH1K73100FB"]
            end
        end

        P2L["pointcloud_to_laserscan_node\npkg: pointcloud_to_laserscan\n(relative topics)"]
        MERGE["rom_pcl_merge\npkg: rom_pcl_filters"]
        TF1["static_transform_publisher\ncamera_broadcaster_01\nbase_link → camera1_link"]
        TF2["static_transform_publisher\ncamera_broadcaster_02\nbase_link → camera2_link"]
        RVIZ["rviz2\n(optional)"]
    end

    style ns fill:#FFF3E0,stroke:#E65100,color:#000
    style CAM1 fill:#C8E6C9,stroke:#2E7D32,color:#000
    style CAM2 fill:#C8E6C9,stroke:#2E7D32,color:#000
    style P2L fill:#FFE0B2,stroke:#E65100,color:#000
    style MERGE fill:#E1BEE7,stroke:#6A1B9A,color:#000
    style TF1 fill:#B2EBF2,stroke:#00838F,color:#000
    style TF2 fill:#B2EBF2,stroke:#00838F,color:#000
    style RVIZ fill:#FFCDD2,stroke:#B71C1C,color:#000
```

---

## Topic Data Flow (with namespace)

> `ROM_ROBOT_NAMESPACE=edu_robot01` ဆိုရင် topics အားလုံး `/edu_robot01/...` prefix ပါလာမည်

```mermaid
graph LR
    CAM1["astra_camera_node\n(camera1)"]
    CAM2["astra_camera_node\n(camera2)"]

    T1(["/<ns>/camera1/depth/points\nPointCloud2"])
    T2(["/<ns>/camera2/depth/points\nPointCloud2"])

    MERGE["rom_pcl_merge"]

    VOXEL(["/<ns>/filter/voxel\nPointCloud2"])

    P2L["pointcloud_to_\nlaserscan_node"]

    SCAN(["/<ns>/camera/scan\nLaserScan"])

    TF1["static_tf\nbase_link → camera1_link"]
    TF2["static_tf\nbase_link → camera2_link"]
    TF_TOPIC(["/tf_static"])

    NAV["Nav2"]
    RVIZ["RViz2"]

    CAM1 --> T1 --> MERGE
    CAM2 --> T2 --> MERGE
    MERGE --> VOXEL --> P2L
    P2L --> SCAN

    TF1 --> TF_TOPIC
    TF2 --> TF_TOPIC

    SCAN --> NAV
    VOXEL --> RVIZ
    SCAN --> RVIZ

    style CAM1 fill:#C8E6C9,stroke:#2E7D32,color:#000
    style CAM2 fill:#C8E6C9,stroke:#2E7D32,color:#000
    style T1 fill:#B2DFDB,stroke:#00695C,color:#000
    style T2 fill:#B2DFDB,stroke:#00695C,color:#000
    style MERGE fill:#E1BEE7,stroke:#6A1B9A,color:#000
    style VOXEL fill:#FFF9C4,stroke:#F57F17,color:#000
    style P2L fill:#FFE0B2,stroke:#E65100,color:#000
    style SCAN fill:#FFCC80,stroke:#EF6C00,color:#000
    style TF1 fill:#B2EBF2,stroke:#00838F,color:#000
    style TF2 fill:#B2EBF2,stroke:#00838F,color:#000
    style TF_TOPIC fill:#B2EBF2,stroke:#00838F,color:#000
    style NAV fill:#D1C4E9,stroke:#4527A0,color:#000
    style RVIZ fill:#FFCDD2,stroke:#B71C1C,color:#000
```

---

## dabai_dw.launch.xml — Single Camera Node

```mermaid
graph LR
    subgraph "dabai_dw.launch.xml"
        ARGS["Args:\ncamera_name\nserial_number\ndevice_num\nvendor_id: 0x2bc5"]
        NODE["astra_camera_node\npkg: astra_camera\nns: (camera_name)"]
    end

    DEPTH(["(camera_name)/depth/raw\nImage"])
    DEPTH_INFO(["(camera_name)/depth/camera_info\nCameraInfo"])
    IR(["(camera_name)/ir/image_raw\nImage"])
    PC(["(camera_name)/depth/points\nPointCloud2"])
    TF(["/tf\nTFMessage"])

    ARGS --> NODE
    NODE --> DEPTH
    NODE --> DEPTH_INFO
    NODE --> IR
    NODE --> PC
    NODE --> TF

    style ARGS fill:#CFD8DC,stroke:#37474F,color:#000
    style NODE fill:#C8E6C9,stroke:#2E7D32,color:#000
    style DEPTH fill:#BBDEFB,stroke:#1565C0,color:#000
    style DEPTH_INFO fill:#BBDEFB,stroke:#1565C0,color:#000
    style IR fill:#E1BEE7,stroke:#6A1B9A,color:#000
    style PC fill:#FFF9C4,stroke:#F57F17,color:#000
    style TF fill:#B2EBF2,stroke:#00838F,color:#000
```

---

## multi_dabai_dw.launch.xml — Multi Camera Include

```mermaid
graph TD
    MULTI["multi_dabai_dw.launch.xml\ndevice_num: 2"]

    DAB1["dabai_dw.launch.xml\ncamera_name: camera1\nserial: CH1K73100C5"]
    DAB2["dabai_dw.launch.xml\ncamera_name: camera2\nserial: CH1K73100FB"]
    DAB3["dabai_dw.launch.xml\ncamera_name: camera3\nserial: CH15120003D\n(commented out)"]

    MULTI -- "include" --> DAB1
    MULTI -- "include" --> DAB2
    MULTI -. "include (disabled)" .-> DAB3

    style MULTI fill:#BBDEFB,stroke:#1565C0,color:#000
    style DAB1 fill:#C8E6C9,stroke:#2E7D32,color:#000
    style DAB2 fill:#C8E6C9,stroke:#2E7D32,color:#000
    style DAB3 fill:#CFD8DC,stroke:#78909C,color:#000
```

---

## Static TF Transforms

```mermaid
graph TD
    BASE["base_link"]
    C1["camera1_link\nxyz: 0.187 0.17 0.203\nrpy: 0.6 -0.6 0.1"]
    C2["camera2_link\nxyz: 0.187 -0.16 0.203\nrpy: -0.7 -0.6 0.0"]

    BASE --> C1
    BASE --> C2

    style BASE fill:#FFCDD2,stroke:#B71C1C,color:#000
    style C1 fill:#C8E6C9,stroke:#2E7D32,color:#000
    style C2 fill:#C8E6C9,stroke:#2E7D32,color:#000
```

---

*ROM Robotics © 2026*