import json
import numpy as np
from scipy.interpolate import interp1d, BPoly

from networkx.algorithms.shortest_paths.generic import shortest_path

from ale.transformation import FrameChain
from ale.base.type_sensor import LineScanner, Framer
from ale.rotation import ConstantRotation, TimeDependentRotation

def to_isd(driver):
    """
    Formatter to create sensor model meta data from a driver.

    Parameters
    ----------
    driver : Driver
        Concrete driver for the image that meta data is being generated for.

    Returns
    -------
    string
        The ISIS compatible meta data as a JSON encoded string.
    """


    meta_data = {}

    meta_data['IsisCameraVersion'] = driver.sensor_model_version

    # interiror orientation
    meta_data['NaifKeywords'] = driver.naif_keywords
    meta_data['detector_sample_summing'] = driver.sample_summing
    meta_data['detector_line_summing'] = driver.line_summing
    meta_data['focal_length_model'] = {
        'focal_length' : driver.focal_length
    }
    meta_data['detector_center'] = {
        'line' : driver.detector_center_line,
        'sample' : driver.detector_center_sample
    }

    meta_data['starting_detector_line'] = driver.detector_start_line
    meta_data['starting_detector_sample'] = driver.detector_start_sample
    meta_data['focal2pixel_lines'] = driver.focal2pixel_lines
    meta_data['focal2pixel_samples'] = driver.focal2pixel_samples
    meta_data['optical_distortion'] = driver.usgscsm_distortion_model

    # general information
    meta_data['image_lines'] = driver.image_lines
    meta_data['image_samples'] = driver.image_samples
    meta_data['name_platform'] = driver.platform_name
    meta_data['name_sensor'] = driver.sensor_name
    meta_data['reference_height'] = {
        "maxheight": 1000,
        "minheight": -1000,
        "unit": "m"
    }

    # line scan sensor model specifics
    if isinstance(driver, LineScanner):
        meta_data['name_model'] = 'USGS_ASTRO_LINE_SCANNER_SENSOR_MODEL'
        meta_data['interpolation_method'] = 'lagrange'

        start_lines, start_times, scan_rates = driver.line_scan_rate
        center_time = driver.center_ephemeris_time
        meta_data['line_scan_rate'] = [[line, time, rate] for line, time, rate in zip(start_lines, start_times, scan_rates)]
        meta_data['starting_ephemeris_time'] = driver.ephemeris_start_time
        meta_data['center_ephemeris_time'] = center_time

    # frame sensor model specifics
    if isinstance(driver, Framer):
        meta_data['name_model'] = 'USGS_ASTRO_FRAME_SENSOR_MODEL'
        meta_data['center_ephemeris_time'] = driver.center_ephemeris_time

    frame_chain = driver.frame_chain
    sensor_frame = driver.sensor_frame_id
    target_frame = driver.target_frame_id

    body_radii = driver.target_body_radii
    meta_data['radii'] = {
        'semimajor' : body_radii[0],
        'semiminor' : body_radii[2],
        'unit' : 'km'
    }

    instrument_pointing = {}
    source_frame, destination_frame, time_dependent_sensor_frame = frame_chain.last_time_dependent_frame_between(1, sensor_frame)

    # Reverse the frame order because ISIS orders frames as
    # (destination, intermediate, ..., intermediate, source)
    instrument_pointing['TimeDependentFrames'] = shortest_path(frame_chain, destination_frame, 1)
    time_dependent_rotation = frame_chain.compute_rotation(1, destination_frame)
    instrument_pointing['CkTableStartTime'] = time_dependent_rotation.times[0]
    instrument_pointing['CkTableEndTime'] = time_dependent_rotation.times[-1]
    instrument_pointing['CkTableOriginalSize'] = len(time_dependent_rotation.times)
    instrument_pointing['EphemerisTimes'] = time_dependent_rotation.times
    instrument_pointing['Quaternions'] = time_dependent_rotation.quats[:, [3, 0, 1, 2]]
    instrument_pointing['AngularVelocity'] = time_dependent_rotation.av

    # Reverse the frame order because ISIS orders frames as
    # (destination, intermediate, ..., intermediate, source)
    instrument_pointing['ConstantFrames'] = shortest_path(frame_chain, sensor_frame, destination_frame)
    constant_rotation = frame_chain.compute_rotation(destination_frame, sensor_frame)
    instrument_pointing['ConstantRotation'] = constant_rotation.rotation_matrix().flatten()
    meta_data['InstrumentPointing'] = instrument_pointing

    body_rotation = {}
    source_frame, destination_frame, time_dependent_target_frame = frame_chain.last_time_dependent_frame_between(target_frame, 1)

    if source_frame != 1:
        # Reverse the frame order because ISIS orders frames as
        # (destination, intermediate, ..., intermediate, source)
        body_rotation['TimeDependentFrames'] = shortest_path(frame_chain, source_frame, 1)
        time_dependent_rotation = frame_chain.compute_rotation(1, source_frame)
        body_rotation['CkTableStartTime'] = time_dependent_rotation.times[0]
        body_rotation['CkTableEndTime'] = time_dependent_rotation.times[-1]
        body_rotation['CkTableOriginalSize'] = len(time_dependent_rotation.times)
        body_rotation['EphemerisTimes'] = time_dependent_rotation.times
        body_rotation['Quaternions'] = time_dependent_rotation.quats[:, [3, 0, 1, 2]]
        body_rotation['AngularVelocity'] = time_dependent_rotation.av

    if source_frame != target_frame:
        # Reverse the frame order because ISIS orders frames as
        # (destination, intermediate, ..., intermediate, source)
        body_rotation['ConstantFrames'] = shortest_path(frame_chain, target_frame, source_frame)
        constant_rotation = frame_chain.compute_rotation(source_frame, target_frame)
        body_rotation['ConstantRotation'] = constant_rotation.rotation_matrix().flatten()

    meta_data['BodyRotation'] = body_rotation

    j2000_rotation = frame_chain.compute_rotation(target_frame, 1)

    instrument_position = {}
    positions, velocities, times = driver.sensor_position
    instrument_position['SpkTableStartTime'] = times[0]
    instrument_position['SpkTableEndTime'] = times[-1]
    instrument_position['SpkTableOriginalSize'] = len(times)
    instrument_position['EphemerisTimes'] = times
    # Rotate positions and velocities into J2000 then scale into kilometers
    velocities = j2000_rotation.rotate_velocity_at(positions, velocities, times)/1000
    positions = j2000_rotation.apply_at(positions, times)/1000
    instrument_position['Positions'] = positions
    instrument_position['Velocities'] = velocities
    meta_data['InstrumentPosition'] = instrument_position

    sun_position = {}
    positions, velocities, times = driver.sun_position
    sun_position['SpkTableStartTime'] = times[0]
    sun_position['SpkTableEndTime'] = times[-1]
    sun_position['SpkTableOriginalSize'] = len(times)
    sun_position['EphemerisTimes'] = times
    # Rotate positions and velocities into J2000 then scale into kilometers
    velocities = j2000_rotation.rotate_velocity_at(positions, velocities, times)/1000
    positions = j2000_rotation.apply_at(positions, times)/1000
    sun_position['Positions'] = positions
    sun_position['Velocities'] = velocities
    meta_data['SunPosition'] = sun_position

    # check that there is a valid sensor model name
    if 'name_model' not in meta_data:
        raise Exception('No CSM sensor model name found!')

    return meta_data
