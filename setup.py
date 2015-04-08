from setuptools import setup

setup(
    name='illacceptanything',
    author='Kristopher Wilson',
    author_email='?',
    version='0.1',
    url='https://github.com/mrkrstphr/illacceptanything',
    packages=['illacceptanything'],
    description='Our standards are incredibly low',
    install_requires=[
        'click',
    ],
    entry_points='''
        [console_scripts]
        illacceptanything=illacceptanything:cli
    ''',

    # classifiers=[
    #     'License :: OSI Approved :: MIT License',
    #     'Programming Language :: Python',
    # ],
)
